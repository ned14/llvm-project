//===-- wg14_signals stdc_raise semantics (hermetic port) -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Hermetic ports of the wg14_signals reference-implementation tests
/// stdc_raise_zero_test.c (analysis.md 1.4), stdc_raise_uninstalled_test.c
/// (2.16/W5), stdc_raise_null_info_test.c (2.14/W2) and
/// out_of_range_signo_test.c (NEGS): the N3924 stdc_raise() contract --
/// setup-call semantics, "no decider installed" returning false without
/// delivery, deterministic zeroed fields for NULL info, and out-of-range
/// signo rejection before the frame walk.
///
//===----------------------------------------------------------------------===//

#include "hdr/errno_macros.h"
#include "hdr/signal_macros.h"
#include "hdr/types/siginfo_t.h"
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
#include "test/UnitTest/Test.h"

#include <stdint.h>

namespace {

#define SIGNAL_TO_USE SIGILL
#define OTHER_SIGNAL SIGABRT

// The documented one-line library setup call: must set up this thread's TLS
// state and return false doing nothing else (analysis.md 1.4).
TEST(LlvmLibcWg14Raise, RaiseZeroIsSetup) {
  EXPECT_FALSE(LIBC_NAMESPACE::stdc_raise(0, nullptr, nullptr));
}

// stdc_raise() of a supported signo with no handler installed returns false
// without delivering the signal (analysis.md 2.16/W5), both directly and
// inside a sigguarded() frame guarding a different signal; once installed
// and claimed it returns true.
static enum sig_decision claiming_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  return sig_decision_resume_execution;
}

static union stdc_siginfo_value noop_recovery(const struct stdc_siginfo *rsi) {
  return rsi->value;
}

static bool guarded_raise_returned_false = false;
static union stdc_siginfo_value guarded_func(union stdc_siginfo_value value) {
  guarded_raise_returned_false =
      !LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr);
  return value;
}

TEST(LlvmLibcWg14Raise, UninstalledRaiseReturnsFalse) {
  (void)LIBC_NAMESPACE::stdc_raise(0, nullptr, nullptr);

  // Install only OTHER_SIGNAL so SIGNAL_TO_USE stays "no handler installed".
  sigset_t base{};
  LIBC_NAMESPACE::sigemptyset(&base);
  LIBC_NAMESPACE::sigaddset(&base, OTHER_SIGNAL);
  void *handlers = LIBC_NAMESPACE::siginstall(&base);
  ASSERT_NE(handlers, nullptr);

  EXPECT_FALSE(LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr));

  // Same raise inside a sigguarded() frame guarding a different signal.
  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, OTHER_SIGNAL);
  union stdc_siginfo_value value{7};
  (void)LIBC_NAMESPACE::sigguarded(&guarded, guarded_func, noop_recovery,
                                   claiming_decider, value);
  EXPECT_TRUE(guarded_raise_returned_false);

  // Sanity: once installed and claimed, stdc_raise must return true.
  sigset_t sanity{};
  LIBC_NAMESPACE::sigemptyset(&sanity);
  LIBC_NAMESPACE::sigaddset(&sanity, SIGNAL_TO_USE);
  void *handlers2 = LIBC_NAMESPACE::siginstall(&sanity);
  ASSERT_NE(handlers2, nullptr);
  sigset_t guarded2{};
  LIBC_NAMESPACE::sigemptyset(&guarded2);
  LIBC_NAMESPACE::sigaddset(&guarded2, SIGNAL_TO_USE);
  void *decider = LIBC_NAMESPACE::signal_decider_create(
      &guarded2, false, claiming_decider, value);
  ASSERT_NE(decider, nullptr);
  EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr));
  EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(decider), 0);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers2), 0);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);
}

// stdc_raise(signo, NULL, NULL) must hand the decider deterministic
// zero/NULL fields (analysis.md 2.14/W2), and a second raise with NULL info
// in the same guarded frame must not hand the decider the first raise's
// stale raw_info.
static struct observed_info {
  int decider_calls;
  int signo;
  int error_code;
  void *addr;
  void *raw_info;
} observed;

static enum sig_decision capture_decider(struct stdc_siginfo *rsi) {
  observed.decider_calls++;
  observed.signo = rsi->signo;
  observed.error_code = rsi->error_code;
  observed.addr = rsi->addr;
  observed.raw_info = rsi->raw_info;
  return sig_decision_resume_execution;
}

static union stdc_siginfo_value
guarded_two_raises(union stdc_siginfo_value value) {
  siginfo_t fake_info;
  __builtin_memset(&fake_info, 0, sizeof(fake_info));
  fake_info.si_addr = reinterpret_cast<void *>(uintptr_t(0x1234));
  (void)LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, &fake_info, nullptr);
  (void)LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr);
  return value;
}

TEST(LlvmLibcWg14Raise, NullInfoGivesZeroedFields) {
  (void)LIBC_NAMESPACE::stdc_raise(0, nullptr, nullptr);

  void *handlers = LIBC_NAMESPACE::siginstall(nullptr);
  ASSERT_NE(handlers, nullptr);

  // Global decider path.
  {
    observed = {};
    sigset_t guarded{};
    LIBC_NAMESPACE::sigemptyset(&guarded);
    LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
    union stdc_siginfo_value value{7};
    void *decider = LIBC_NAMESPACE::signal_decider_create(
        &guarded, false, capture_decider, value);
    ASSERT_NE(decider, nullptr);
    EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr));
    EXPECT_EQ(observed.decider_calls, 1);
    EXPECT_EQ(observed.signo, SIGNAL_TO_USE);
    EXPECT_EQ(observed.error_code, 0);
    EXPECT_EQ(observed.addr, nullptr);
    EXPECT_EQ(observed.raw_info, nullptr);
    EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(decider), 0);
  }

  // Frame path: a second raise with NULL info in the same guarded frame must
  // not hand the decider the first raise's stale raw_info pointer.
  {
    observed = {};
    sigset_t guarded{};
    LIBC_NAMESPACE::sigemptyset(&guarded);
    LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
    union stdc_siginfo_value value{7};
    (void)LIBC_NAMESPACE::sigguarded(&guarded, guarded_two_raises,
                                     noop_recovery, capture_decider, value);
    EXPECT_EQ(observed.decider_calls, 2);
    EXPECT_EQ(observed.signo, SIGNAL_TO_USE);
    EXPECT_EQ(observed.error_code, 0);
    EXPECT_EQ(observed.addr, nullptr);
    EXPECT_EQ(observed.raw_info, nullptr);
  }

  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);
}

// Out-of-range/negative signos must be rejected before the frame walk
// (analysis.md NEGS): the frame decider must never be invoked, and the
// documented "no decider installed for that signal" false is returned.
static int frame_decider_calls = 0;
static bool sanity_claimed = false;
static bool neg_returned_false = false;
static bool pos_returned_false = false;
static bool far_returned_false = false;

static enum sig_decision frame_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  frame_decider_calls++;
  return sig_decision_resume_execution;
}

static union stdc_siginfo_value
guarded_out_of_range(union stdc_siginfo_value value) {
  // Sanity: a real raise of the guarded signal is claimed by the frame
  // decider.
  sanity_claimed = LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr);
  frame_decider_calls = 0;
  neg_returned_false = !LIBC_NAMESPACE::stdc_raise(-1, nullptr, nullptr);
  pos_returned_false = !LIBC_NAMESPACE::stdc_raise(NSIG, nullptr, nullptr);
  far_returned_false = !LIBC_NAMESPACE::stdc_raise(NSIG + 10, nullptr, nullptr);
  return value;
}

TEST(LlvmLibcWg14Raise, OutOfRangeSignoRejected) {
  (void)LIBC_NAMESPACE::stdc_raise(0, nullptr, nullptr);

  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
  void *handlers = LIBC_NAMESPACE::siginstall(&guarded);
  ASSERT_NE(handlers, nullptr);
  union stdc_siginfo_value value{7};
  (void)LIBC_NAMESPACE::sigguarded(&guarded, guarded_out_of_range,
                                   noop_recovery, frame_decider, value);
  EXPECT_TRUE(sanity_claimed);
  EXPECT_TRUE(neg_returned_false);
  EXPECT_TRUE(pos_returned_false);
  EXPECT_TRUE(far_returned_false);
  EXPECT_EQ(frame_decider_calls, 0);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);
}

} // namespace
