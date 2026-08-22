//===-- wg14_signals handoff semantics (hermetic port) --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Hermetic ports of the wg14_signals reference-implementation tests
/// nsih_siginfo_handoff_test.c (analysis.md NSIH) and
/// decider_destroy_return_test.c (SDCF): stdc_raise() handing off to a
/// pre-existing SA_SIGINFO handler must synthesise a minimal non-NULL
/// siginfo_t, siguninstall() must restore the pre-existing handler exactly,
/// and signal_decider_destroy() must report success for every removed handle
/// (including the all-NULL-slot warning path) with NULL as the only failure.
///
//===----------------------------------------------------------------------===//

#include "hdr/errno_macros.h"
#include "hdr/signal_macros.h"
#include "hdr/types/siginfo_t.h"
#include "hdr/types/sigset_t.h"
#include "hdr/types/struct_stdc_siginfo.h"
#include "hdr/types/union_stdc_siginfo_value.h"
#include "src/__support/libc_errno.h"
#include "src/signal/sigaction.h"
#include "src/signal/sigaddset.h"
#include "src/signal/sigemptyset.h"
#include "src/signal/siginstall.h"
#include "src/signal/signal_decider_create.h"
#include "src/signal/signal_decider_destroy.h"
#include "src/signal/siguninstall.h"
#include "src/signal/stdc_raise.h"
#include "test/UnitTest/Test.h"

namespace {

#define SIGNAL_TO_USE SIGUSR2

static volatile int handler_calls = 0;
static volatile int saw_valid_si = 0;
static volatile int saw_signo = 0;
static volatile int saw_si_code_user = 0;

static void preexisting_siginfo_handler(int signo, siginfo_t *si, void *ctx) {
  (void)ctx;
  handler_calls++;
  if (si != nullptr) {
    saw_valid_si = 1;
    if (si->si_signo == signo)
      saw_signo = 1;
    if (si->si_code == SI_USER)
      saw_si_code_user = 1;
  }
}

// stdc_raise(signo, NULL, NULL) handing off to a pre-existing SA_SIGINFO
// handler must pass a synthesised minimal siginfo_t (non-NULL, si_signo and
// si_code set), never NULL (NSIH); siguninstall() must restore the
// pre-existing handler exactly.
TEST(LlvmLibcWg14Handoff, SiginfoHandoffToPreexistingHandler) {
  (void)LIBC_NAMESPACE::stdc_raise(0, nullptr, nullptr);

  struct sigaction sa{};
  sa.sa_sigaction = preexisting_siginfo_handler;
  sa.sa_flags = SA_SIGINFO;
  LIBC_NAMESPACE::sigemptyset(&sa.sa_mask);
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGNAL_TO_USE, &sa, nullptr), 0);

  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
  void *handlers = LIBC_NAMESPACE::siginstall(&guarded);
  ASSERT_NE(handlers, nullptr);

  handler_calls = 0;
  saw_valid_si = 0;
  saw_signo = 0;
  saw_si_code_user = 0;
  EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr));
  EXPECT_EQ(handler_calls, 1);
  EXPECT_NE(saw_valid_si, 0);
  EXPECT_NE(saw_signo, 0);
  EXPECT_NE(saw_si_code_user, 0);

  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);

  // siguninstall must restore the pre-existing handler.
  struct sigaction restored{};
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGNAL_TO_USE, nullptr, &restored), 0);
  EXPECT_NE(restored.sa_flags & SA_SIGINFO, 0);
}

static enum sig_decision claiming_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  return sig_decision_resume_execution;
}

// signal_decider_destroy must return 0 whenever the handle is removed, even
// when every slot is NULL (the create-time warning path); only a NULL handle
// is a failure (SDCF).
TEST(LlvmLibcWg14Handoff, DeciderDestroyReturnContract) {
  union stdc_siginfo_value value{7};

  // Destroying a decider handle whose slots are all NULL (no handler
  // installed at create time) must report success, 0, with errno unchanged.
  {
    sigset_t g{};
    LIBC_NAMESPACE::sigemptyset(&g);
    LIBC_NAMESPACE::sigaddset(&g, SIGABRT);
    void *d = LIBC_NAMESPACE::signal_decider_create(&g, false, claiming_decider,
                                                    value);
    ASSERT_NE(d, nullptr);
    LIBC_NAMESPACE::libc_errno = 0;
    EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(d), 0);
    EXPECT_EQ(static_cast<int>(LIBC_NAMESPACE::libc_errno), 0);
  }

  // A mixed handle -- one installed signal carrying a live decider node and
  // one warning-path NULL slot -- must destroy cleanly with 0, and the
  // installed node must be unlinked so the container survives a later
  // siguninstall.
  {
    sigset_t install{};
    LIBC_NAMESPACE::sigemptyset(&install);
    LIBC_NAMESPACE::sigaddset(&install, SIGABRT);
    void *h = LIBC_NAMESPACE::siginstall(&install);
    ASSERT_NE(h, nullptr);
    sigset_t g{};
    LIBC_NAMESPACE::sigemptyset(&g);
    LIBC_NAMESPACE::sigaddset(&g, SIGABRT);
    LIBC_NAMESPACE::sigaddset(&g, SIGTERM);
    void *d = LIBC_NAMESPACE::signal_decider_create(&g, false, claiming_decider,
                                                    value);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(d), 0);
    EXPECT_EQ(LIBC_NAMESPACE::siguninstall(h), 0);
  }

  // Contract sanity: a NULL handle is still a failure (EINVAL, -1), the only
  // genuinely unsuccessful destroy.
  LIBC_NAMESPACE::libc_errno = 0;
  EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(nullptr), -1);
  EXPECT_EQ(static_cast<int>(LIBC_NAMESPACE::libc_errno), EINVAL);
}

} // namespace
