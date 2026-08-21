//===-- Embedder override layer for the wg14_signals submodule --*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Embedder override hooks for the wg14_signals reference implementation
/// (WG14 N3924 improved C signals), mounted as a pristine git submodule at
/// libc/src/signal/wg14.
///
/// The submodule's POSIX backend calls, by name, the very functions this
/// libc is implementing on top of it: sigaction(), abort() and
/// pthread_kill(pthread_self(), ...). If those calls resolved to libc's
/// public entrypoints, the library would recurse into itself (sigaction()
/// calling sigaction(), raise() calling stdc_raise()). The submodule's
/// config.h therefore defines embedder override hooks
/// (WG14_SIGNALS_SIGACTION / WG14_SIGNALS_ABORT / WG14_SIGNALS_KILL_SELF /
/// WG14_SIGNALS_GETTID) defaulting to the standard calls; this header
/// redefines them to libc's own kernel-facing layer, implemented in
/// embedder_shim.c. It must be included BEFORE wg14_signals/config.h, so it
/// is force-included (-include) ahead of every submodule translation unit.
///
/// This file is part of the fork, not the submodule: the submodule itself is
/// never patched.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_SIGNAL_WG14_EMBEDDER_SHIM_H
#define LLVM_LIBC_SRC_SIGNAL_WG14_EMBEDDER_SHIM_H

#include <pthread.h> // pthread_key_t, pthread_once_t
#include <setjmp.h>  // jmp_buf
#include <signal.h>
#include <stddef.h> // size_t

#ifdef LIBC_FULL_BUILD
// In the full build the shim routes the hooks straight to the kernel (raw
// syscalls), never through libc's public signal entrypoints. In overlay
// builds there is no recursion hazard (libc's entrypoints are not the
// registry) and the submodule resolves the defaults against the host libc,
// so no overrides are defined and the shim source is compiled out.

#ifdef __cplusplus
extern "C" {
#endif

int wg14_embedder_sigaction(int signum, const struct sigaction *act,
                            struct sigaction *oldact);
void wg14_embedder_abort(void);
int wg14_embedder_kill_self(int signo);
long wg14_embedder_gettid(void);

// Host-call bridge (wg14_hostcalls.cpp): routes the submodule's remaining
// host calls (memory, sigset helpers, setjmp/longjmp, pthread keys,
// allocator) to libc's own entrypoints. Implemented in a separate object
// so its own references only matter when the wg14 objects are actually
// pulled into a link.
void *wg14_hostcall_memcpy(void *dest, const void *src, size_t n);
void *wg14_hostcall_memset(void *dest, int c, size_t n);
void *wg14_hostcall_malloc(size_t n);
void *wg14_hostcall_calloc(size_t n, size_t s);
void wg14_hostcall_free(void *p);
int wg14_hostcall_sigemptyset(sigset_t *set);
int wg14_hostcall_sigfillset(sigset_t *set);
int wg14_hostcall_sigaddset(sigset_t *set, int signo);
int wg14_hostcall_sigdelset(sigset_t *set, int signo);
int wg14_hostcall_sigismember(const sigset_t *set, int signo);
int wg14_hostcall_pthread_key_create(pthread_key_t *key, void (*dtor)(void *));
int wg14_hostcall_pthread_once(pthread_once_t *once, void (*init)(void));
int wg14_hostcall_pthread_setspecific(pthread_key_t key, const void *value);
void *wg14_hostcall_pthread_getspecific(pthread_key_t key);
int wg14_hostcall_stderr_printf(const char *format, ...)
    __attribute__((format(printf, 1, 2)));

#ifdef __cplusplus
}
#endif

#define WG14_SIGNALS_SIGACTION(signum, act, oldact)                            \
  wg14_embedder_sigaction(signum, act, oldact)
#define WG14_SIGNALS_ABORT() wg14_embedder_abort()
#define WG14_SIGNALS_KILL_SELF(signo) wg14_embedder_kill_self(signo)
#define WG14_SIGNALS_GETTID() wg14_embedder_gettid()
#define WG14_SIGNALS_MEMCPY(dest, src, n)                                      \
  wg14_hostcall_memcpy((dest), (src), (n))
#define WG14_SIGNALS_MEMSET(dest, c, n) wg14_hostcall_memset((dest), (c), (n))
#define WG14_SIGNALS_MALLOC(n) wg14_hostcall_malloc(n)
#define WG14_SIGNALS_CALLOC(n, s) wg14_hostcall_calloc((n), (s))
#define WG14_SIGNALS_FREE(p) wg14_hostcall_free(p)
#define WG14_SIGNALS_SIGEMPTYSET(set) wg14_hostcall_sigemptyset(set)
#define WG14_SIGNALS_SIGFILLSET(set) wg14_hostcall_sigfillset(set)
#define WG14_SIGNALS_SIGADDSET(set, signo)                                     \
  wg14_hostcall_sigaddset((set), (signo))
#define WG14_SIGNALS_SIGDELSET(set, signo)                                     \
  wg14_hostcall_sigdelset((set), (signo))
#define WG14_SIGNALS_SIGISMEMBER(set, signo)                                   \
  wg14_hostcall_sigismember((set), (signo))
#define WG14_SIGNALS_STDERR_PRINTF(...) wg14_hostcall_stderr_printf(__VA_ARGS__)
#define WG14_SIGNALS_PTHREAD_KEY_CREATE(key, dtor)                             \
  wg14_hostcall_pthread_key_create((key), (dtor))
#define WG14_SIGNALS_PTHREAD_ONCE(once, init)                                  \
  wg14_hostcall_pthread_once((once), (init))
#define WG14_SIGNALS_PTHREAD_SETSPECIFIC(key, value)                           \
  wg14_hostcall_pthread_setspecific((key), (value))
#define WG14_SIGNALS_PTHREAD_GETSPECIFIC(key)                                  \
  wg14_hostcall_pthread_getspecific(key)
#define WG14_SIGNALS_SETJMP(buf) setjmp(buf)
#define WG14_SIGNALS_LONGJMP(buf, val) longjmp((buf), (val))
#endif // LIBC_FULL_BUILD

#endif // LLVM_LIBC_SRC_SIGNAL_WG14_EMBEDDER_SHIM_H
