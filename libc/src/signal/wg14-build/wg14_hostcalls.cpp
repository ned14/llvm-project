//===-- wg14_signals host-call bridge --------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Implementations of the wg14_signals embedder override hooks for the
/// host-call families beyond the recursion-critical ones (memory, sigset
/// helpers, setjmp/longjmp, pthread keys, allocator). Each hook is an
/// extern "C" function declared in embedder_shim.h (which the wg14 object
/// library force-includes), routing to libc's own C++ entrypoints via their
/// namespace-scope declarations, so the references resolve in both the
/// library build (public entrypoint objects, which define both the C and
/// the namespace symbols) and the hermetic test builds (internal objects,
/// which define the namespace symbols).
///
/// The allocator hooks route to the C symbols malloc/calloc/free: the
/// allocator (scudo) is an external runtime, so tests that pull the wg14
/// machinery must link it (the libc test rules do when the allocator
/// entrypoints are in the config and the test depends on them).
///
/// `__assert_fail` (assert.h's failure path) cannot be routed to the C++
/// entrypoint in test builds, so a weak definition is provided here; the
/// strong libc entrypoint wins in the library build.
///
/// This file is part of the fork, not the submodule: the submodule itself
/// is never patched.
///
//===----------------------------------------------------------------------===//

#include "embedder_shim.h"

#ifdef LIBC_FULL_BUILD

#include "hdr/types/sigset_t.h"
#include "src/__support/OSUtil/exit.h"    // LIBC_NAMESPACE::internal::exit
#include "src/__support/OSUtil/syscall.h" // LIBC_NAMESPACE::syscall_impl
#include "src/__support/common.h"
#include "src/__support/macros/config.h"
#include "src/pthread/pthread_getspecific.h"
#include "src/pthread/pthread_key_create.h"
#include "src/pthread/pthread_once.h"
#include "src/pthread/pthread_setspecific.h"
#include "src/setjmp/longjmp.h"
#include "src/setjmp/setjmp_impl.h"
#include "src/signal/sigaddset.h"
#include "src/signal/sigdelset.h"
#include "src/signal/sigemptyset.h"
#include "src/signal/sigfillset.h"
#include "src/signal/sigismember.h"
#include "src/stdio/stderr.h"
#include "src/stdio/vfprintf.h"
#include "src/string/memcpy.h"
#include "src/string/memset.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>      // malloc/calloc/free (C symbols; scudo)
#include <sys/syscall.h> // SYS_write

extern "C" {

void *wg14_hostcall_memcpy(void *dest, const void *src, size_t n) {
  return LIBC_NAMESPACE::memcpy(dest, src, n);
}

void *wg14_hostcall_memset(void *dest, int c, size_t n) {
  return LIBC_NAMESPACE::memset(dest, c, n);
}

void *wg14_hostcall_malloc(size_t n) { return malloc(n); }

void *wg14_hostcall_calloc(size_t n, size_t s) { return calloc(n, s); }

void wg14_hostcall_free(void *p) { free(p); }

int wg14_hostcall_sigemptyset(sigset_t *set) {
  return LIBC_NAMESPACE::sigemptyset(set);
}

int wg14_hostcall_sigfillset(sigset_t *set) {
  return LIBC_NAMESPACE::sigfillset(set);
}

int wg14_hostcall_sigaddset(sigset_t *set, int signo) {
  return LIBC_NAMESPACE::sigaddset(set, signo);
}

int wg14_hostcall_sigdelset(sigset_t *set, int signo) {
  return LIBC_NAMESPACE::sigdelset(set, signo);
}

int wg14_hostcall_sigismember(const sigset_t *set, int signo) {
  return LIBC_NAMESPACE::sigismember(set, signo);
}

int wg14_hostcall_pthread_key_create(pthread_key_t *key, void (*dtor)(void *)) {
  return LIBC_NAMESPACE::pthread_key_create(key, dtor);
}

int wg14_hostcall_pthread_once(pthread_once_t *once, void (*init)(void)) {
  return LIBC_NAMESPACE::pthread_once(once, init);
}

int wg14_hostcall_pthread_setspecific(pthread_key_t key, const void *value) {
  return LIBC_NAMESPACE::pthread_setspecific(key, value);
}

void *wg14_hostcall_pthread_getspecific(pthread_key_t key) {
  return LIBC_NAMESPACE::pthread_getspecific(key);
}

// Weak C setjmp/longjmp so hermetic test links (whose internal libc objects
// provide only the namespace-scope symbols) resolve the wg14 objects' direct
// calls; the strong libc entrypoints win in the library build. These weak
// bridges are NOT used in the library build (the direct naked setjmp is
// correct there); in test builds they only satisfy link-time resolution.
__attribute__((weak)) int setjmp(jmp_buf buf) {
  return LIBC_NAMESPACE::setjmp(buf);
}

__attribute__((weak, noreturn)) void longjmp(jmp_buf buf, int val) {
  LIBC_NAMESPACE::longjmp(buf, val);
  __builtin_unreachable();
}

// Weak malloc/calloc/free so hermetic test links resolve the wg14 objects'
// allocator calls (the libc test framework does not link the scudo runtime
// into unit tests). The strong allocator (scudo) wins in the library build;
// the fallback below is a correct page-granular allocator over the raw mmap
// syscall (zeroed pages make calloc trivial), async-signal-safe and
// thread-safe. It exists only to keep test links functional; the library
// build never executes it.
struct wg14_alloc_header {
  size_t size;
};

__attribute__((weak)) void *malloc(size_t n) {
  const size_t page = 4096;
  size_t total = n + sizeof(struct wg14_alloc_header);
  size_t rounded = (total + page - 1) & ~(page - 1);
  long ret = LIBC_NAMESPACE::syscall_impl<long>(
      SYS_mmap, 0, rounded, 3 /* PROT_READ|PROT_WRITE */,
      0x22 /* MAP_PRIVATE|MAP_ANONYMOUS */, -1, 0);
  if (ret < 0)
    return nullptr;
  void *p = (void *)ret;
  ((struct wg14_alloc_header *)p)->size = rounded;
  return (char *)p + sizeof(struct wg14_alloc_header);
}

__attribute__((weak)) void *calloc(size_t n, size_t s) {
  size_t total;
  if (__builtin_mul_overflow(n, s, &total))
    return nullptr;
  // mmap pages are zeroed.
  return malloc(total);
}

__attribute__((weak)) void free(void *p) {
  if (p == nullptr)
    return;
  struct wg14_alloc_header *h =
      (struct wg14_alloc_header *)((char *)p -
                                   sizeof(struct wg14_alloc_header));
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_munmap, h, h->size);
}

// The submodule's WG14_SIGNALS_STDERR_PRINTF default (fprintf(stderr, ...))
// routes through this bridge so the diagnostics resolve in both the library
// and the hermetic test builds (stderr/fprintf are in the death-test dep
// set of the test framework).
int wg14_hostcall_stderr_printf(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int r = LIBC_NAMESPACE::vfprintf(LIBC_NAMESPACE::stderr, format, ap);
  va_end(ap);
  return r;
}

// Weak __assert_fail so hermetic test links that pull the wg14 objects
// (whose <assert.h> assert() calls the C symbol) resolve; the strong libc
// entrypoint wins in the library build.
__attribute__((weak)) void __assert_fail(const char *assertion,
                                         const char *file, unsigned int line,
                                         const char *function) {
  // Async-signal-safe string length (no libc dependency).
  auto slen = [](const char *str) -> size_t {
    size_t n = 0;
    while (str[n] != '\0')
      ++n;
    return n;
  };
  const char msg[] = "assertion failed: ";
  const char nl[] = "\n";
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, msg, sizeof(msg) - 1);
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, assertion,
                                           slen(assertion));
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, " at ", 4);
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, file, slen(file));
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, ":", 1);
  char linebuf[24];
  // Small decimal conversion (async-signal-safe; no libc dependency).
  unsigned int v = line;
  char *p = linebuf + sizeof(linebuf);
  *--p = '\0';
  do {
    *--p = (char)('0' + (v % 10));
    v /= 10;
  } while (v != 0);
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, p, slen(p));
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, " in ", 4);
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, function,
                                           slen(function));
  (void)LIBC_NAMESPACE::syscall_impl<long>(SYS_write, 2, nl, sizeof(nl) - 1);
  LIBC_NAMESPACE::internal::exit(127);
}

} // extern "C"

#endif // LIBC_FULL_BUILD
