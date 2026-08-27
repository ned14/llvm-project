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

#include <string.h>
#include <sys/syscall.h> // SYS_rt_sigaction, SYS_gettid, ...

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
