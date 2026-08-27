//===-- wg14_signals install semantics (hermetic port) --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Hermetic ports of the wg14_signals reference-implementation tests
/// siginstall_null_set_test.c (analysis.md RTIM), edge_api_coverage_test.c
/// (TCOV) and sig_default_preserve_test.c (DFLT): the siginstall() NULL-set
/// contract (all standard signals, excluding libc-internal and realtime
/// ranges), the failure/edge APIs (siguninstall_system, the asynchronous
/// sigfillset helpers, signal_decider_create error paths), and the SIG_DFL
/// default-action re-raise preserving the library's installed handler.
///
//===----------------------------------------------------------------------===//

#include "hdr/errno_macros.h"
#include "hdr/signal_macros.h"
#include "hdr/types/sigset_t.h"
#include "hdr/types/struct_stdc_siginfo.h"
#include "hdr/types/union_stdc_siginfo_value.h"
#include "src/__support/libc_errno.h"
#include "src/signal/sigaction.h"
#include "src/signal/sigaddset.h"
#include "src/signal/sigemptyset.h"
#include "src/signal/sigfillset.h"
#include "src/signal/sigfillset_asynchronous_debug.h"
#include "src/signal/sigfillset_asynchronous_nondebug.h"
#include "src/signal/siginstall.h"
#include "src/signal/sigismember.h"
#include "src/signal/signal_decider_create.h"
#include "src/signal/siguninstall.h"
#include "src/signal/siguninstall_system.h"
#include "src/signal/stdc_raise.h"
#include "test/UnitTest/Test.h"

namespace {

// siginstall(NULL) must return a handle covering the standard signals but
// excluding libc-internal signals (SIGCANCEL/SIGSETXID) and the realtime
// range; the same filtering applies to explicit guarded inputs (RTIM).
TEST(LlvmLibcWg14Install, NullSetCoversStandardExcludesInternal) {
  void *set = LIBC_NAMESPACE::siginstall(nullptr);
  ASSERT_NE(set, nullptr);

  EXPECT_EQ(LIBC_NAMESPACE::sigismember((const sigset_t *)set, SIGILL), 1);
  EXPECT_EQ(LIBC_NAMESPACE::sigismember((const sigset_t *)set, SIGTERM), 1);
  EXPECT_EQ(LIBC_NAMESPACE::sigismember((const sigset_t *)set, SIGSEGV), 1);
#ifdef SIGUSR1
  EXPECT_EQ(LIBC_NAMESPACE::sigismember((const sigset_t *)set, SIGUSR1), 1);
#endif
#ifdef SIGCANCEL
  EXPECT_NE(LIBC_NAMESPACE::sigismember((const sigset_t *)set, SIGCANCEL), 1);
#endif
#ifdef SIGSETXID
  EXPECT_NE(LIBC_NAMESPACE::sigismember((const sigset_t *)set, SIGSETXID), 1);
#endif
#if defined(SIGRTMIN) && defined(SIGRTMAX)
  for (int signo = (int)SIGRTMIN; signo <= (int)SIGRTMAX; signo++)
    EXPECT_NE(LIBC_NAMESPACE::sigismember((const sigset_t *)set, signo), 1);
#endif

  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(set), 0);

  // Explicit guarded input is filtered for the libc-internal signals too.
  sigset_t guarded{};
  LIBC_NAMESPACE::sigfillset(&guarded);
  void *explicit_handle = LIBC_NAMESPACE::siginstall(&guarded);
  ASSERT_NE(explicit_handle, nullptr);
#ifdef SIGCANCEL
  EXPECT_NE(
      LIBC_NAMESPACE::sigismember((const sigset_t *)explicit_handle, SIGCANCEL),
      1);
#endif
#ifdef SIGSETXID
  EXPECT_NE(
      LIBC_NAMESPACE::sigismember((const sigset_t *)explicit_handle, SIGSETXID),
      1);
#endif
  EXPECT_EQ(
      LIBC_NAMESPACE::sigismember((const sigset_t *)explicit_handle, SIGILL),
      1);
  EXPECT_EQ(
      LIBC_NAMESPACE::sigismember((const sigset_t *)explicit_handle, SIGTERM),
      1);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(explicit_handle), 0);
}

static enum sig_decision never_called_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  return sig_decision_next_decider;
}

// The failure/edge APIs: asynchronous sigfillset helpers, siguninstall_system
// and the signal_decider_create error paths (TCOV).
TEST(LlvmLibcWg14Install, EdgeApis) {
  sigset_t nondebug{};
  EXPECT_EQ(LIBC_NAMESPACE::sigfillset_asynchronous_nondebug(&nondebug), 0);
  // The non-debug asynchronous set must not contain synchronous-fault
  // signals: SIGSEGV is a member of the synchronous set only.
  EXPECT_NE(LIBC_NAMESPACE::sigismember(&nondebug, SIGSEGV), 1);
  sigset_t debug{};
  EXPECT_EQ(LIBC_NAMESPACE::sigfillset_asynchronous_debug(&debug), 0);

  EXPECT_EQ(LIBC_NAMESPACE::siguninstall_system(0), 0);
  LIBC_NAMESPACE::libc_errno = 0;
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall_system(1), -1);
  EXPECT_EQ(static_cast<int>(LIBC_NAMESPACE::libc_errno), EINVAL);

  union stdc_siginfo_value value{};

  // NULL guarded set.
  LIBC_NAMESPACE::libc_errno = 0;
  EXPECT_EQ(LIBC_NAMESPACE::signal_decider_create(nullptr, false,
                                                  never_called_decider, value),
            nullptr);
  EXPECT_EQ(static_cast<int>(LIBC_NAMESPACE::libc_errno), EINVAL);

  // Empty guarded set (no signal members).
  sigset_t empty{};
  LIBC_NAMESPACE::sigemptyset(&empty);
  LIBC_NAMESPACE::libc_errno = 0;
  EXPECT_EQ(LIBC_NAMESPACE::signal_decider_create(&empty, false,
                                                  never_called_decider, value),
            nullptr);
  EXPECT_EQ(static_cast<int>(LIBC_NAMESPACE::libc_errno), EINVAL);

  // Non-empty guarded set but NULL decider.
  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGILL);
  LIBC_NAMESPACE::libc_errno = 0;
  EXPECT_EQ(
      LIBC_NAMESPACE::signal_decider_create(&guarded, false, nullptr, value),
      nullptr);
  EXPECT_EQ(static_cast<int>(LIBC_NAMESPACE::libc_errno), EINVAL);
}

// SIG_DFL default-action re-raise must restore the library's installed
// handler, not leave the kernel handler reset to SIG_DFL (DFLT). SIGCONT's
// default action never blocks a running test process.
TEST(LlvmLibcWg14Install, DefaultActionPreservesInstalledHandler) {
  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGCONT);
  void *handlers = LIBC_NAMESPACE::siginstall(&guarded);
  ASSERT_NE(handlers, nullptr);

  struct sigaction before{};
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGCONT, nullptr, &before), 0);
  // The library's handler must be installed for SIGCONT before the raise.
  EXPECT_NE(before.sa_handler, SIG_DFL);
  EXPECT_NE(before.sa_handler, SIG_IGN);

  // No decider claims the raise, so stdc_raise() falls through to the
  // previously installed (SIG_DFL) handler. Taking that default action must
  // not permanently discard the library's installed handler.
  EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGCONT, nullptr, nullptr));

  struct sigaction after{};
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGCONT, nullptr, &after), 0);
  EXPECT_NE(after.sa_handler, SIG_DFL);
  EXPECT_NE(after.sa_handler, SIG_IGN);
  // The very same handler object survived the default-action re-raise.
  EXPECT_EQ(after.sa_handler, before.sa_handler);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);
}

} // namespace
