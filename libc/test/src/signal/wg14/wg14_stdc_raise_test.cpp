//===-- wg14_signals stdc_raise semantics (hermetic port) ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Hermetic port of the wg14_signals stdc_raise_test.c (N3924 7.14.2.10).
/// stdc_raise() behaves as if it called raise(), except that the caller's
/// raw_info/raw_context is made available to the signal deciders; a NULL
/// argument is replaced by a constructed implementation-defined default so a
/// decider never sees a NULL raw_info or raw_context. The signal is delivered
/// for real through the kernel.
///
//===----------------------------------------------------------------------===//

#include "hdr/signal_macros.h"
#include "hdr/types/siginfo_t.h"
#include "hdr/types/sigset_t.h"
#include "hdr/types/struct_stdc_siginfo.h"
#include "hdr/types/union_stdc_siginfo_value.h"
#include "src/signal/sigaddset.h"
#include "src/signal/sigemptyset.h"
#include "src/signal/sighandle.h"
#include "src/signal/siginstall.h"
#include "src/signal/signal_decider_create.h"
#include "src/signal/signal_decider_destroy.h"
#include "src/signal/siguninstall.h"
#include "src/signal/stdc_raise.h"
#include "test/UnitTest/Test.h"

#include <stdint.h>

namespace {

#define SIGNAL_TO_USE SIGILL

static struct Observed {
  int calls = 0;
  int signo = 0;
  int error_code = 0;
  void *addr = nullptr;
  void *raw_info = nullptr;
  void *raw_context = nullptr;
  int raw_si_signo = 0;
  int raw_si_code = 0;
} observed;

static enum sig_decision capture_decider(struct stdc_siginfo *rsi) {
  observed.calls++;
  observed.signo = rsi->signo;
  observed.error_code = rsi->error_code;
  observed.addr = rsi->addr;
  observed.raw_info = rsi->raw_info;
  observed.raw_context = rsi->raw_context;
  if (rsi->raw_info != nullptr) {
    observed.raw_si_signo = static_cast<siginfo_t *>(rsi->raw_info)->si_signo;
    observed.raw_si_code = static_cast<siginfo_t *>(rsi->raw_info)->si_code;
  }
  return sig_decision_resume_execution;
}

// Not a valid signal number: unsuccessful, and no signal is raised
// (7.14.2.10 p8).
TEST(LlvmLibcWg14StdcRaise, RejectsInvalidSigno) {
  EXPECT_NE(LIBC_NAMESPACE::stdc_raise(0, nullptr, nullptr), 0);
  EXPECT_NE(LIBC_NAMESPACE::stdc_raise(-1, nullptr, nullptr), 0);
  EXPECT_NE(LIBC_NAMESPACE::stdc_raise(NSIG, nullptr, nullptr), 0);
}

// A signal this library has not installed is raised for real as raise() would:
// no decider runs, and SIGCONT's default action for a running process is to do
// nothing.
TEST(LlvmLibcWg14StdcRaise, RaisesUninstalledSignal) {
  siginfo_t fake_info{};
  EXPECT_EQ(LIBC_NAMESPACE::stdc_raise(SIGCONT, &fake_info, nullptr), 0);
}

// The documented signo == 0 setup form, then a real raise whose decider sees
// exactly the caller-supplied raw_info/raw_context (7.14.2.10 p4), and a
// NULL/NULL raise whose decider sees a constructed default (7.14.2.10
// p5/p6/p7).
TEST(LlvmLibcWg14StdcRaise, CallerInfoAndConstructedDefault) {
  (void)LIBC_NAMESPACE::sighandle(0, nullptr, nullptr);
  void *handlers = LIBC_NAMESPACE::siginstall(nullptr);
  ASSERT_NE(handlers, nullptr);

  union stdc_siginfo_value value{7};
  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
  void *decider = LIBC_NAMESPACE::signal_decider_create(&guarded, false,
                                                        capture_decider, value);
  ASSERT_NE(decider, nullptr);

  // Caller-supplied raw_info/raw_context are surfaced unchanged.
  {
    observed = {};
    siginfo_t fake_info{};
    fake_info.si_signo = SIGNAL_TO_USE;
    fake_info.si_code = SI_QUEUE;
    fake_info.si_errno = 3;
    fake_info.si_addr = reinterpret_cast<void *>(uintptr_t(0x5678));
    int fake_context = 42;
    EXPECT_EQ(
        LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, &fake_info, &fake_context),
        0);
    EXPECT_EQ(observed.calls, 1);
    EXPECT_EQ(observed.signo, SIGNAL_TO_USE);
    EXPECT_EQ(observed.raw_info, &fake_info);
    EXPECT_EQ(observed.raw_context, &fake_context);
    EXPECT_EQ(observed.error_code, fake_info.si_errno);
    EXPECT_EQ(observed.addr, fake_info.si_addr);
  }

  // NULL raw_info/raw_context is replaced by a constructed default: raw_info
  // and raw_context are never NULL, the default siginfo carries si_signo and
  // SI_USER, error_code is zero and addr is NULL.
  {
    observed = {};
    EXPECT_EQ(LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr), 0);
    EXPECT_EQ(observed.calls, 1);
    EXPECT_NE(observed.raw_info, nullptr);
    EXPECT_NE(observed.raw_context, nullptr);
    EXPECT_EQ(observed.raw_si_signo, SIGNAL_TO_USE);
    EXPECT_EQ(observed.raw_si_code, SI_USER);
    EXPECT_EQ(observed.error_code, 0);
    EXPECT_EQ(observed.addr, nullptr);
  }

  EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(decider), 0);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);
}

} // namespace
