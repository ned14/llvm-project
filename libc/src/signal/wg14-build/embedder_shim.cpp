//===-- Embedder shim implementation for the wg14_signals submodule -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Implementation of the embedder override hooks declared in embedder_shim.h.
/// Every function here is async-signal-safe and thread-safe: it performs raw
/// kernel syscalls only (never libc signal entrypoints), so the wg14_signals
/// backend cannot recurse through the libc API surface it is built on.
///
/// This file is part of the fork, not the submodule: the submodule itself is
/// never patched.
///
//===----------------------------------------------------------------------===//

#include "embedder_shim.h"

#ifdef LIBC_FULL_BUILD

#include "hdr/signal_macros.h"
#include "hdr/types/sigset_t.h"
#include "hdr/types/struct_sigaction.h"
#include "src/__support/OSUtil/exit.h"    // LIBC_NAMESPACE::internal::exit
#include "src/__support/OSUtil/syscall.h" // LIBC_NAMESPACE::syscall_impl
#include "src/__support/common.h"
#include "src/__support/macros/config.h"

#include <stdint.h>
#include <string.h>
#include <sys/syscall.h> // SYS_rt_sigaction, SYS_gettid, ...
#include <ucontext.h>    // ucontext_t, mcontext_t (getcontext shim)

// The kernel's rt_sigaction ABI struct (Linux uapi asm-generic/signal.h):
// {handler-union, sa_flags, sa_restorer, sa_mask} - "mask last for
// extensibility". This differs from the userspace struct sigaction
// ({handler-union, sa_mask, sa_flags, sa_restorer}) used by libc's public
// header and by wg14_signals, so both directions of the syscall convert.
struct wg14_embedder_kernel_sigaction {
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
  };
  unsigned long sa_flags;
  void (*sa_restorer)(void);
  sigset_t sa_mask;
};

// The rt_sigreturn restorer stub (libc/src/signal/linux/__restore.cpp); the
// kernel runs it to return from a delivered handler when SA_RESTORER is set.
extern "C" void __restore_rt(void);

extern "C" int wg14_embedder_sigaction(int signum, const struct sigaction *act,
                                       struct sigaction *oldact) {
  struct wg14_embedder_kernel_sigaction kact;
  struct wg14_embedder_kernel_sigaction kold;
  if (act != nullptr) {
    kact.sa_flags = act->sa_flags;
    kact.sa_restorer = act->sa_restorer;
    kact.sa_mask = act->sa_mask;
    if (act->sa_flags & SA_SIGINFO)
      kact.sa_sigaction = act->sa_sigaction;
    else
      kact.sa_handler = act->sa_handler;
    if (!(kact.sa_flags & SA_RESTORER)) {
      // libc is freestanding and does not rely on the VDSO restorer, so an
      // explicit restorer is required when installing a handler.
      kact.sa_flags |= SA_RESTORER;
      kact.sa_restorer = __restore_rt;
    }
    // The wg14_signals raw handler is installed with SA_NOCLDWAIT (the
    // reference implementation never reaps children). A full C library must
    // preserve POSIX wait() semantics, so strip the flag: children of the
    // process then become zombies under the SIGCHLD default disposition
    // exactly as with glibc, and waitpid()/wait4() keep working.
    kact.sa_flags &= ~SA_NOCLDWAIT;
  }
  long ret = LIBC_NAMESPACE::syscall_impl<long>(
      SYS_rt_sigaction, signum, act != nullptr ? &kact : nullptr,
      oldact != nullptr ? &kold : nullptr, sizeof(sigset_t));
  if (ret < 0) {
    LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2,
                                       "DBG: shim sigaction FAILED\n", 26);
    return -1; // errno was set by the syscall wrapper
  }
  if (oldact != nullptr) {
    oldact->sa_flags = (int)kold.sa_flags;
    oldact->sa_restorer = kold.sa_restorer;
    oldact->sa_mask = kold.sa_mask;
    if (kold.sa_flags & SA_SIGINFO)
      oldact->sa_sigaction = kold.sa_sigaction;
    else
      oldact->sa_handler = kold.sa_handler;
  }
  return 0;
}

extern "C" int wg14_embedder_kill_self(int signo) {
  long pid = LIBC_NAMESPACE::syscall_impl<long>(SYS_getpid);
  if (pid < 0)
    return -1;
  long tid = LIBC_NAMESPACE::syscall_impl<long>(SYS_gettid);
  if (tid < 0)
    return -1;
  long ret = LIBC_NAMESPACE::syscall_impl<long>(SYS_tgkill, pid, tid, signo);
  return ret < 0 ? -1 : 0;
}

extern "C" long wg14_embedder_gettid(void) {
  return LIBC_NAMESPACE::syscall_impl<long>(SYS_gettid);
}

// Thread-directed self-delivery carrying a token in si_value (N3924
// stdc_raise()'s raw_info/raw_context matching, config.h
// WG14_SIGNALS_RAISE_WITH_TOKEN). Mirrors the reference POSIX backend's
// raise_with_token(), which uses syscall(SYS_rt_tgsigqueueinfo) -- libc has
// no public syscall(), so go through the kernel-facing wrapper.
extern "C" int wg14_embedder_raise_with_token(int signo, union sigval value) {
  siginfo_t si;
  memset(&si, 0, sizeof(si));
  si.si_signo = signo;
  si.si_code = SI_QUEUE;
  si.si_value = value;
  long pid = LIBC_NAMESPACE::syscall_impl<long>(SYS_getpid);
  if (pid < 0)
    return -1;
  long tid = LIBC_NAMESPACE::syscall_impl<long>(SYS_gettid);
  if (tid < 0)
    return -1;
  long ret = LIBC_NAMESPACE::syscall_impl<long>(SYS_rt_tgsigqueueinfo, pid, tid,
                                                signo, &si);
  return ret < 0 ? -1 : 0;
}

// The reference's default-context construction (N3924 7.14.2.9 p8) uses
// getcontext(); libc implements getcontext() on x86_64 only, so capture the
// calling frame here on every target (the embedder_shim.h
// WG14_SIGNALS_GETCONTEXT macro routes the reference's call to this function).
// The captured object just has to represent the calling stack frame and be
// distinguishable from an all-zero object.
extern "C" int wg14_embedder_getcontext(void *ucp) {
  ucontext_t *uc = (ucontext_t *)ucp;
  memset(uc, 0, sizeof(*uc));
#if defined(__x86_64__)
  uintptr_t sp;
  __asm__ volatile("movq %%rsp, %0" : "=r"(sp));
  uc->uc_mcontext.gregs[REG_RSP] = (greg_t)sp;
  uc->uc_mcontext.gregs[REG_RBP] =
      (greg_t)(uintptr_t)__builtin_frame_address(0);
  uc->uc_mcontext.gregs[REG_RIP] =
      (greg_t)(uintptr_t)__builtin_return_address(0);
#elif defined(__aarch64__)
  unsigned long long sp;
  __asm__ volatile("mov %0, sp" : "=r"(sp));
  uc->uc_mcontext.sp = sp;
  uc->uc_mcontext.regs[29] =
      (unsigned long long)(uintptr_t)__builtin_frame_address(0);
  uc->uc_mcontext.regs[30] =
      (unsigned long long)(uintptr_t)__builtin_return_address(0);
  uc->uc_mcontext.pc =
      (unsigned long long)(uintptr_t)__builtin_return_address(0);
#else
#error "wg14_embedder_getcontext: unsupported target architecture"
#endif
  return 0;
}

extern "C" void wg14_embedder_abort(void) {
  // C11 abort() semantics without any libc signal entrypoint: raise SIGABRT
  // with the current disposition (a catching handler runs), and if the
  // handler returns, reset to SIG_DFL and raise again; finally unblock
  // SIGABRT (the pending delivery then terminates the process) and exit(127)
  // as the hard-abort fallback. This mirrors libc's abort_utils::abort()
  // (Phase 2.6 reworks that to route through the wg14 registry; the shim
  // stays the recursion-free last resort).
  struct sigaction cur;
  memset(&cur, 0, sizeof(cur));
  if (wg14_embedder_sigaction(SIGABRT, nullptr, &cur) == 0) {
    if (cur.sa_handler != SIG_DFL && cur.sa_handler != SIG_IGN) {
      // Deliver in-process to the installed handler.
      if (cur.sa_flags & SA_SIGINFO)
        cur.sa_sigaction(SIGABRT, nullptr, nullptr);
      else
        cur.sa_handler(SIGABRT);
    }
  }
  struct sigaction dfl;
  memset(&dfl, 0, sizeof(dfl));
  dfl.sa_handler = SIG_DFL;
  (void)wg14_embedder_sigaction(SIGABRT, &dfl, nullptr);
  (void)wg14_embedder_kill_self(SIGABRT);
  // Unblock SIGABRT so the pending default delivery terminates the process.
  sigset_t set;
  memset(&set, 0, sizeof(set));
  set.__signals[(SIGABRT - 1) / (8 * sizeof(unsigned long))] |=
      (1UL << ((SIGABRT - 1) % (8 * sizeof(unsigned long))));
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_rt_sigprocmask, SIG_UNBLOCK,
                                           &set, nullptr, sizeof(sigset_t));
  LIBC_NAMESPACE::internal::exit(127);
}

#endif // LIBC_FULL_BUILD
