//===-- sigfence macro definitions ------------------------------*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the sigfence macro (WG14 N3924 improved C signals). The
/// machinery is extracted verbatim from the wg14_signals reference
/// implementation (libc/src/signal/wg14/include/wg14_signals/
/// thrd_signal_handle.h); keep the two in sync (a sync test diffs them).
/// Define LLVM_LIBC_DISABLE_SIGFENCE_MACRO before including <signal.h> to
/// suppress it (the wg14_signals headers honour
/// WG14_SIGNALS_DISABLE_SIGFENCE_MACRO instead).
//
#ifndef LLVM_LIBC_DISABLE_SIGFENCE_MACRO
#define LLVM_LIBC_SIGFENCE_GLUE(x, y) x y
#define LLVM_LIBC_SIGFENCE_RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, _6_, _7_, \
                                            _8_, count, ...)                   \
  count
#define LLVM_LIBC_SIGFENCE_EXPAND_ARGS(args)                                   \
  LLVM_LIBC_SIGFENCE_RETURN_ARG_COUNT args

// The argument counting below uses __VA_OPT__ only for the zero-argument
// sigfence() form (the comma-suppression case it exists for). __VA_OPT__ is
// C23/C++20, provided as an extension by GCC/Clang in all modes and by MSVC's
// conforming preprocessor in C++20 and C11/C17 modes only. MSVC in C++14/17
// mode has no __VA_OPT__ at all, so use a plain comma-list counting there:
// it dispatches 1..8 arguments correctly (and the compile-time assert below
// checks exactly those), while the zero-argument sigfence() form is
// unavailable on such compilers (plans/analysis.md 4.10).
#if defined(__GNUC__) || defined(__clang__)
#define LLVM_LIBC_SIGFENCE_HAVE_VA_OPT 1
#elif defined(_MSC_VER) && defined(_MSVC_TRADITIONAL) &&                       \
    (0 == _MSVC_TRADITIONAL)
#if defined(__cplusplus)
#if defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L)
#define LLVM_LIBC_SIGFENCE_HAVE_VA_OPT 1
#endif
#else
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define LLVM_LIBC_SIGFENCE_HAVE_VA_OPT 1
#endif
#endif
#endif
#ifdef LLVM_LIBC_SIGFENCE_HAVE_VA_OPT
#define LLVM_LIBC_SIGFENCE_COUNT_ARGS_MAX8(...)                                \
  LLVM_LIBC_SIGFENCE_EXPAND_ARGS(                                              \
      (__VA_ARGS__ __VA_OPT__(, ) 8, 7, 6, 5, 4, 3, 2, 1, 0))
#else
#define LLVM_LIBC_SIGFENCE_COUNT_ARGS_MAX8(...)                                \
  LLVM_LIBC_SIGFENCE_EXPAND_ARGS((__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#endif
#define LLVM_LIBC_SIGFENCE_OVERLOAD_MACRO2(name, count) name##count
#define LLVM_LIBC_SIGFENCE_OVERLOAD_MACRO1(name, count)                        \
  LLVM_LIBC_SIGFENCE_OVERLOAD_MACRO2(name, count)
#define LLVM_LIBC_SIGFENCE_OVERLOAD_MACRO(name, count)                         \
  LLVM_LIBC_SIGFENCE_OVERLOAD_MACRO1(name, count)
#define LLVM_LIBC_SIGFENCE_CALL_OVERLOAD(name, ...)                            \
  LLVM_LIBC_SIGFENCE_GLUE(                                                     \
      LLVM_LIBC_SIGFENCE_OVERLOAD_MACRO(                                       \
          name, LLVM_LIBC_SIGFENCE_COUNT_ARGS_MAX8(__VA_ARGS__)),              \
      (__VA_ARGS__))

// The arg counting above depends on __VA_OPT__ (C23/C++20, supported as a
// GNU/Clang/MSVC extension in older modes). Verify at compile time that the
// counting machinery returns the right counts: on a compiler without
// __VA_OPT__ the expansion below is a hard preprocessing error, and on a
// compiler whose counting is broken the assertion fails — either way the
// defect surfaces instead of silently mis-dispatching. (The zero-argument
// path — the comma-suppression case __VA_OPT__ exists for — is exercised by
// test/sigfence_fence_test.c, whose TU suppresses the -Wpedantic diagnostic
// that an empty variadic-macro call triggers; a public header must not.
// The counts asserted here — 3 and 5 — are in the 1..8 range that the
// non-__VA_OPT__ fallback counting also handles, so the assert is valid on
// every supported compiler, including MSVC C++14/17.)
_Static_assert(
    ((LLVM_LIBC_SIGFENCE_COUNT_ARGS_MAX8(a, b, c) == 3) &&
     (LLVM_LIBC_SIGFENCE_COUNT_ARGS_MAX8(a, b, c, d, e) == 5)),
    "wg14_signals: sigfence() argument counting is broken (requires __VA_OPT__ "
    "on this compiler, or a counting-broken preprocessor)");

#if (defined(__GNUC__) || defined(__clang__)) && !defined(DISABLE_INLINE_ASM)
// On compilers with extended inline asm, we can tell the compiler that a
// specific list of variables must be specifically written out and reloaded
// around the fence. You may find https://godbolt.org/z/chh8ee6Mj useful to
// review. DISABLE_INLINE_ASM (defined by cmake/filc-toolchain.cmake for the
// Fil-C memory-safe compiler) selects the portable volatile-sink fallback
// below instead, because Fil-C cannot compile these asm forms (analysis.md
// AA2).
#define LLVM_LIBC_SIGFENCE_IMPL_0() __asm__ volatile(";" : : : "memory")
#define LLVM_LIBC_SIGFENCE_IMPL_1(a)                                           \
  __asm__ volatile(";" : "+m"(a) : : "memory")
#define LLVM_LIBC_SIGFENCE_IMPL_2(a, b)                                        \
  __asm__ volatile(";" : "+m"(a), "+m"(b) : : "memory")
#define LLVM_LIBC_SIGFENCE_IMPL_3(a, b, c)                                     \
  __asm__ volatile(";" : "+m"(a), "+m"(b), "+m"(c) : : "memory")
#define LLVM_LIBC_SIGFENCE_IMPL_4(a, b, c, d)                                  \
  __asm__ volatile(";" : "+m"(a), "+m"(b), "+m"(c), "+m"(d) : : "memory")
#define LLVM_LIBC_SIGFENCE_IMPL_5(a, b, c, d, e)                               \
  __asm__ volatile(";"                                                         \
                   : "+m"(a), "+m"(b), "+m"(c), "+m"(d), "+m"(e)               \
                   :                                                           \
                   : "memory")
#define LLVM_LIBC_SIGFENCE_IMPL_6(a, b, c, d, e, f)                            \
  __asm__ volatile(";"                                                         \
                   : "+m"(a), "+m"(b), "+m"(c), "+m"(d), "+m"(e), "+m"(f)      \
                   :                                                           \
                   : "memory")
#define LLVM_LIBC_SIGFENCE_IMPL_7(a, b, c, d, e, f, g)                         \
  __asm__ volatile(";"                                                         \
                   : "+m"(a), "+m"(b), "+m"(c), "+m"(d), "+m"(e), "+m"(f),     \
                     "+m"(g)                                                   \
                   :                                                           \
                   : "memory")
#define LLVM_LIBC_SIGFENCE_IMPL_8(a, b, c, d, e, f, g, h)                      \
  __asm__ volatile(";"                                                         \
                   : "+m"(a), "+m"(b), "+m"(c), "+m"(d), "+m"(e), "+m"(f),     \
                     "+m"(g), "+m"(h)                                          \
                   :                                                           \
                   : "memory")
#else
// Compilers without extended inline asm (e.g. MSVC), or with it disabled via
// -DDISABLE_INLINE_ASM (Fil-C): force the listed local variables to be
// memory-resident, and their values reloaded afterwards, as the "+m" operands
// and "memory" clobber above do.
// LLVM_LIBC_SIGFENCE_ESCAPE() (1) stores each variable's address into a
// volatile sink so the address escapes to observable memory, and (2) performs
// a volatile read of one byte of the object -- char may alias any object
// (C11 6.5p7), and volatile accesses are observable behaviour (C11 5.1.2.3),
// so no optimizer, link-time code generation (/GL /LTCG) included, may
// eliminate or reorder them: the value is committed to memory before the
// fence and must be reloaded after it. No out-of-line function is needed, so
// the fence cannot be defeated by the optimizer inlining a helper away, and
// the sink is per-TU static, so header-only consumers need no library
// symbols.
static void *volatile llvm_libc_sigfence_sink[9];
#define LLVM_LIBC_SIGFENCE_BARRIER()                                           \
  ((void)(llvm_libc_sigfence_sink[8] = llvm_libc_sigfence_sink[8]))
#define LLVM_LIBC_SIGFENCE_ESCAPE(a, i)                                        \
  do {                                                                         \
    llvm_libc_sigfence_sink[(i)] = (void *)&(a);                               \
    (void)*(volatile unsigned char *)&(a);                                     \
  } while (0)
#define LLVM_LIBC_SIGFENCE_IMPL_0() LLVM_LIBC_SIGFENCE_BARRIER()
#define LLVM_LIBC_SIGFENCE_IMPL_1(a)                                           \
  do {                                                                         \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
    LLVM_LIBC_SIGFENCE_ESCAPE(a, 0);                                           \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
  } while (0)
#define LLVM_LIBC_SIGFENCE_IMPL_2(a, b)                                        \
  do {                                                                         \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
    LLVM_LIBC_SIGFENCE_ESCAPE(a, 0);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(b, 1);                                           \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
  } while (0)
#define LLVM_LIBC_SIGFENCE_IMPL_3(a, b, c)                                     \
  do {                                                                         \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
    LLVM_LIBC_SIGFENCE_ESCAPE(a, 0);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(b, 1);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(c, 2);                                           \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
  } while (0)
#define LLVM_LIBC_SIGFENCE_IMPL_4(a, b, c, d)                                  \
  do {                                                                         \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
    LLVM_LIBC_SIGFENCE_ESCAPE(a, 0);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(b, 1);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(c, 2);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(d, 3);                                           \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
  } while (0)
#define LLVM_LIBC_SIGFENCE_IMPL_5(a, b, c, d, e)                               \
  do {                                                                         \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
    LLVM_LIBC_SIGFENCE_ESCAPE(a, 0);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(b, 1);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(c, 2);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(d, 3);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(e, 4);                                           \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
  } while (0)
#define LLVM_LIBC_SIGFENCE_IMPL_6(a, b, c, d, e, f)                            \
  do {                                                                         \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
    LLVM_LIBC_SIGFENCE_ESCAPE(a, 0);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(b, 1);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(c, 2);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(d, 3);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(e, 4);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(f, 5);                                           \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
  } while (0)
#define LLVM_LIBC_SIGFENCE_IMPL_7(a, b, c, d, e, f, g)                         \
  do {                                                                         \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
    LLVM_LIBC_SIGFENCE_ESCAPE(a, 0);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(b, 1);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(c, 2);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(d, 3);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(e, 4);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(f, 5);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(g, 6);                                           \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
  } while (0)
#define LLVM_LIBC_SIGFENCE_IMPL_8(a, b, c, d, e, f, g, h)                      \
  do {                                                                         \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
    LLVM_LIBC_SIGFENCE_ESCAPE(a, 0);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(b, 1);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(c, 2);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(d, 3);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(e, 4);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(f, 5);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(g, 6);                                           \
    LLVM_LIBC_SIGFENCE_ESCAPE(h, 7);                                           \
    LLVM_LIBC_SIGFENCE_BARRIER();                                              \
  } while (0)
#endif
//! \brief A compiler-only memory barrier, including for local variables in the
//! argument list. Any variable in the argument list MUST be a lvalue.
#define sigfence(...)                                                          \
  LLVM_LIBC_SIGFENCE_CALL_OVERLOAD(LLVM_LIBC_SIGFENCE_IMPL_, __VA_ARGS__)

#endif // LLVM_LIBC_DISABLE_SIGFENCE_MACRO
