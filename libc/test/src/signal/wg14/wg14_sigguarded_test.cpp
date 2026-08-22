//===-- wg14_signals sigguarded semantics (hermetic port) -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Hermetic ports of the wg14_signals reference-implementation tests
/// recovery_null_loop_test.c (analysis.md 1.7) and sigguarded_tss_init_test.c
/// (2.19/X3): a sigguarded() frame whose decider asks for recovery but has no
/// recovery function must fall through to the outer frame (never re-fault
/// forever), and sigguarded() must set up the per-thread state so a global
/// decider claiming a raise out of a frame works.
///
//===----------------------------------------------------------------------===//

#include "hdr/errno_macros.h"
#include "hdr/signal_macros.h"
#include "hdr/types/sigset_t.h"
#include "hdr/types/struct_stdc_siginfo.h"
#include "hdr/types/union_stdc_siginfo_value.h"
#include "src/signal/sigaddset.h"
#include "src/signal/sigemptyset.h"
#include "src/signal/sigguarded.h"
#include "src/signal/siginstall.h"
#include "src/signal/signal_decider_create.h"
#include "src/signal/signal_decider_destroy.h"
#include "src/signal/siguninstall.h"
#include "src/signal/stdc_raise.h"
#include "src/stdlib/_Exit.h"
#include "test/UnitTest/Test.h"

#include <stdint.h>

namespace {

#ifndef SIGSEGV
#define SIGSEGV 11
#endif

// A store to this address faults; volatile and opaque so the compiler cannot
// fold it into a null pointer constant and warn, or elide the store.
static volatile uintptr_t bad_address = 0;

struct shared_t {
  volatile int inner_decider_calls;
  int outer_decider_calls;
  int outer_recovery_calls;
  int recovered_signo;
};

// The inner decider asks to invoke recovery, but the inner sigguarded has no
// recovery function. Before the analysis.md 1.7 fix the exception was sent
// back to the faulting instruction, which re-faulted forever; detect that
// loop and fail the test instead of hanging.
static enum sig_decision inner_decider_func(struct stdc_siginfo *rsi) {
  struct shared_t *shared =
      static_cast<struct shared_t *>(rsi->value.ptr_value);
  if (++shared->inner_decider_calls > 100)
    LIBC_NAMESPACE::_Exit(2);
  return sig_decision_call_recovery;
}

static enum sig_decision outer_decider_func(struct stdc_siginfo *rsi) {
  struct shared_t *shared =
      static_cast<struct shared_t *>(rsi->value.ptr_value);
  shared->outer_decider_calls++;
  return sig_decision_call_recovery;
}

static union stdc_siginfo_value
outer_recovery_func(const struct stdc_siginfo *rsi) {
  struct shared_t *shared =
      static_cast<struct shared_t *>(rsi->value.ptr_value);
  shared->outer_recovery_calls++;
  shared->recovered_signo = rsi->signo;
  return rsi->value;
}

// Trigger a genuine synchronous fault. The kernel re-executes this
// instruction whenever the library handler returns without recovering, which
// is exactly the 1.7 infinite fault loop.
__attribute__((no_sanitize("address",
                           "undefined"))) static union stdc_siginfo_value
fault_func(union stdc_siginfo_value value) {
  (void)value;
  *(volatile int *)bad_address = 1;
  return value;
}

// The inner sigguarded guards SIGSEGV but supplies recovery == NULL, the 1.7
// trigger.
static union stdc_siginfo_value call_inner(union stdc_siginfo_value value) {
  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGSEGV);
  return LIBC_NAMESPACE::sigguarded(&guarded, fault_func, nullptr,
                                    inner_decider_func, value);
}

// A genuine SIGSEGV in the innermost frame with no recovery must fall through
// to the outer frame's recovery instead of looping (analysis.md 1.7).
TEST(LlvmLibcWg14Sigguarded, NullRecoveryFallsThrough) {
  void *handlers = LIBC_NAMESPACE::siginstall(nullptr);
  ASSERT_NE(handlers, nullptr);

  struct shared_t shared{};
  union stdc_siginfo_value value;
  value.ptr_value = &shared;
  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGSEGV);
  (void)LIBC_NAMESPACE::sigguarded(&guarded, call_inner, outer_recovery_func,
                                   outer_decider_func, value);

  EXPECT_EQ(shared.outer_recovery_calls, 1);
  EXPECT_EQ(shared.recovered_signo, SIGSEGV);
  EXPECT_EQ(shared.outer_decider_calls, 1);
  EXPECT_EQ(shared.inner_decider_calls, 1);

  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);
}

// The documented failure sentinel of sigguarded() (N3924 7.14.4.1).
TEST(LlvmLibcWg14Sigguarded, FailureValueSentinel) {
  EXPECT_EQ(static_cast<int>(SIGGUARDED_FAILURE_VALUE.int_value), -99);
}

// Deliberately do NOT call stdc_raise(0, ...) anywhere here: that setup call
// initialises the per-thread TSS, which this test must leave uninitialised
// until the sigguarded() under test (analysis.md 2.19/X3).
static int global_decider_called = 0;

static enum sig_decision declining_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  return sig_decision_next_decider;
}

static enum sig_decision claiming_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  global_decider_called++;
  return sig_decision_resume_execution;
}

static union stdc_siginfo_value noop_recovery(const struct stdc_siginfo *rsi) {
  return rsi->value;
}

static union stdc_siginfo_value guarded_raises(union stdc_siginfo_value value) {
  (void)LIBC_NAMESPACE::stdc_raise(SIGSEGV, nullptr, nullptr);
  return value;
}

// sigguarded() must set up the per-thread TSS so a raise that the frame
// decider declines propagates to the global decider without crashing
// (analysis.md 2.19/X3).
TEST(LlvmLibcWg14Sigguarded, TssInitBeforeGlobalClaim) {
  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGSEGV);
  void *handlers = LIBC_NAMESPACE::siginstall(&guarded);
  ASSERT_NE(handlers, nullptr);
  union stdc_siginfo_value value{7};
  void *decider = LIBC_NAMESPACE::signal_decider_create(
      &guarded, false, claiming_decider, value);
  ASSERT_NE(decider, nullptr);

  global_decider_called = 0;
  const union stdc_siginfo_value result = LIBC_NAMESPACE::sigguarded(
      &guarded, guarded_raises, noop_recovery, declining_decider, value);
  EXPECT_EQ(global_decider_called, 1);
  EXPECT_EQ(static_cast<int>(result.int_value), 7);

  EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(decider), 0);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);
}

} // namespace
