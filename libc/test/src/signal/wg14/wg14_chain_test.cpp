//===-- wg14_signals coexistence with libc entrypoints --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// libc-native coexistence tests (Phase 3 step 2): the public POSIX signal
/// entrypoints and the wg14 N3924 entrypoints sharing a signal, and the
/// abort()/sigguarded interplay. Handler chaining (user sigaction, then
/// siginstall wrapping it, a decider claiming or declining, then
/// siguninstall restoring the user handler exactly), altstack delivery
/// through the handoff, and abort() re-entry via a sigguarded(SIGABRT)
/// recovery frame.
///
//===----------------------------------------------------------------------===//

#include "hdr/errno_macros.h"
#include "hdr/signal_macros.h"
#include "hdr/types/siginfo_t.h"
#include "hdr/types/sigset_t.h"
#include "hdr/types/struct_stdc_siginfo.h"
#include "hdr/types/union_stdc_siginfo_value.h"
#include "src/signal/raise.h"
#include "src/signal/sigaction.h"
#include "src/signal/sigaddset.h"
#include "src/signal/sigaltstack.h"
#include "src/signal/sigemptyset.h"
#include "src/signal/sigguarded.h"
#include "src/signal/siginstall.h"
#include "src/signal/signal_decider_create.h"
#include "src/signal/signal_decider_destroy.h"
#include "src/signal/siguninstall.h"
#include "src/signal/stdc_raise.h"
#include "src/stdlib/abort.h"
#include "test/UnitTest/Test.h"

#include <stdint.h>

namespace {

// Handler chaining: a user handler installed with sigaction, wrapped by
// siginstall, claimed by a decider, then declining -- and finally restored
// exactly by siguninstall.
static int user_handler_calls = 0;
static void user_handler(int signo) {
  if (signo == SIGUSR1)
    user_handler_calls++;
}

static enum sig_decision claiming_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  return sig_decision_resume_execution;
}

static enum sig_decision declining_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  return sig_decision_next_decider;
}

TEST(LlvmLibcWg14Chain, UserHandlerWrappedClaimedDeclinedRestored) {
  struct sigaction sa{};
  sa.sa_handler = user_handler;
  LIBC_NAMESPACE::sigemptyset(&sa.sa_mask);
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGUSR1, &sa, nullptr), 0);

  sigset_t one{};
  LIBC_NAMESPACE::sigemptyset(&one);
  LIBC_NAMESPACE::sigaddset(&one, SIGUSR1);
  void *inst = LIBC_NAMESPACE::siginstall(&one);
  ASSERT_NE(inst, nullptr);

  // A decider claims the raise: the user handler must not run.
  union stdc_siginfo_value value{};
  void *dec = LIBC_NAMESPACE::signal_decider_create(&one, false,
                                                    claiming_decider, value);
  ASSERT_NE(dec, nullptr);
  user_handler_calls = 0;
  EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGUSR1, nullptr, nullptr));
  EXPECT_EQ(user_handler_calls, 0);
  EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(dec), 0);

  // With no decider claiming, the raise hands off to the user handler.
  user_handler_calls = 0;
  EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGUSR1, nullptr, nullptr));
  EXPECT_EQ(user_handler_calls, 1);

  // A declining decider also lets the raise reach the user handler.
  dec = LIBC_NAMESPACE::signal_decider_create(&one, false, declining_decider,
                                              value);
  ASSERT_NE(dec, nullptr);
  user_handler_calls = 0;
  EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGUSR1, nullptr, nullptr));
  EXPECT_EQ(user_handler_calls, 1);
  EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(dec), 0);

  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(inst), 0);

  // siguninstall restored the user handler exactly: raise() runs it again.
  user_handler_calls = 0;
  EXPECT_EQ(LIBC_NAMESPACE::raise(SIGUSR1), 0);
  EXPECT_EQ(user_handler_calls, 1);

  struct sigaction restored{};
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGUSR1, nullptr, &restored), 0);
  EXPECT_EQ(restored.sa_handler, user_handler);

  // Cleanup for other tests.
  sa.sa_handler = SIG_DFL;
  LIBC_NAMESPACE::sigaction(SIGUSR1, &sa, nullptr);
}

// SA_SIGINFO fidelity through the wg14 handoff: a pre-existing SA_SIGINFO
// handler receives a synthesised siginfo_t with its own si_signo and
// si_code == SI_USER.
static volatile int sig_calls = 0;
static volatile bool sig_saw_signo = false;
static volatile bool sig_saw_code_user = false;

static void sig_handler(int signo, siginfo_t *si, void *ctx) {
  (void)ctx;
  sig_calls++;
  if (si != nullptr && si->si_signo == signo)
    sig_saw_signo = 1;
  if (si != nullptr && si->si_code == SI_USER)
    sig_saw_code_user = 1;
}

TEST(LlvmLibcWg14Chain, SiginfoFidelityThroughHandoff) {
  (void)LIBC_NAMESPACE::stdc_raise(0, nullptr, nullptr);

  struct sigaction sa{};
  sa.sa_sigaction = sig_handler;
  sa.sa_flags = SA_SIGINFO;
  LIBC_NAMESPACE::sigemptyset(&sa.sa_mask);
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGUSR2, &sa, nullptr), 0);

  sigset_t one{};
  LIBC_NAMESPACE::sigemptyset(&one);
  LIBC_NAMESPACE::sigaddset(&one, SIGUSR2);
  void *inst = LIBC_NAMESPACE::siginstall(&one);
  ASSERT_NE(inst, nullptr);

  sig_calls = 0;
  sig_saw_signo = 0;
  sig_saw_code_user = 0;
  EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGUSR2, nullptr, nullptr));
  EXPECT_EQ(sig_calls, 1);
  EXPECT_TRUE(sig_saw_signo);
  EXPECT_TRUE(sig_saw_code_user);

  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(inst), 0);
  sa.sa_handler = SIG_DFL;
  LIBC_NAMESPACE::sigaction(SIGUSR2, &sa, nullptr);
}

// Altstack delivery: the kernel path (raise()) with SA_ONSTACK runs the
// handler on the alternate stack. The wg14 handoff, by contrast, invokes the
// previously installed handler SYNCHRONOUSLY (invoke_sigaction calls it
// directly -- no kernel re-delivery), so that leg must assert the handler
// ran, not where it ran.
constexpr int LOCAL_VAR_SIZE = 512;
constexpr int ALT_STACK_SIZE = SIGSTKSZ + LOCAL_VAR_SIZE * 2;
static uint8_t alt_stack[ALT_STACK_SIZE];

static bool good_stack;
static int altstack_handler_calls = 0;
static void altstack_handler(int) {
  altstack_handler_calls++;
  uint8_t var[LOCAL_VAR_SIZE];
  for (int i = 0; i < LOCAL_VAR_SIZE; ++i)
    var[i] = static_cast<uint8_t>(i);
  for (int i = 0; i < LOCAL_VAR_SIZE; ++i) {
    if (!(uintptr_t(var + i) < uintptr_t(alt_stack + ALT_STACK_SIZE) &&
          uintptr_t(alt_stack) <= uintptr_t(var + i))) {
      good_stack = false;
      return;
    }
  }
  good_stack = true;
}

TEST(LlvmLibcWg14Chain, AltstackDeliveryThroughHandoff) {
  struct sigaction action{};
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGUSR1, nullptr, &action), 0);
  action.sa_handler = altstack_handler;
  action.sa_flags = SA_ONSTACK;
  ASSERT_EQ(LIBC_NAMESPACE::sigaction(SIGUSR1, &action, nullptr), 0);

  stack_t ss{};
  ss.ss_sp = alt_stack;
  ss.ss_size = ALT_STACK_SIZE;
  ss.ss_flags = 0;
  ASSERT_EQ(LIBC_NAMESPACE::sigaltstack(&ss, nullptr), 0);

  // Direct kernel delivery (no wg14 involvement) runs on the altstack.
  good_stack = false;
  altstack_handler_calls = 0;
  EXPECT_EQ(LIBC_NAMESPACE::raise(SIGUSR1), 0);
  EXPECT_TRUE(good_stack);
  EXPECT_EQ(altstack_handler_calls, 1);

  // The same SA_ONSTACK handler wrapped by siginstall: an unclaimed
  // stdc_raise hands off to the saved handler synchronously; the handler
  // must run (on the current stack -- invoke_sigaction does not re-enter the
  // kernel).
  sigset_t one{};
  LIBC_NAMESPACE::sigemptyset(&one);
  LIBC_NAMESPACE::sigaddset(&one, SIGUSR1);
  void *inst = LIBC_NAMESPACE::siginstall(&one);
  ASSERT_NE(inst, nullptr);
  altstack_handler_calls = 0;
  EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGUSR1, nullptr, nullptr));
  EXPECT_EQ(altstack_handler_calls, 1);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(inst), 0);

  // Restore defaults and clear the altstack for other tests.
  action.sa_handler = SIG_DFL;
  LIBC_NAMESPACE::sigaction(SIGUSR1, &action, nullptr);
  stack_t clear{};
  clear.ss_flags = SS_DISABLE;
  LIBC_NAMESPACE::sigaltstack(&clear, nullptr);
}

// abort() re-entry: with SIGABRT siginstall'ed and a sigguarded frame whose
// decider asks for recovery, abort() routes through stdc_raise, the frame
// longjmps out and the recovery function runs -- abort() never returns and
// the process survives.
static volatile bool recovery_ran = false;

static union stdc_siginfo_value recovery_fn(const struct stdc_siginfo *rsi) {
  (void)rsi;
  recovery_ran = 1;
  return SIGGUARDED_FAILURE_VALUE;
}

static enum sig_decision recovery_decider(struct stdc_siginfo *rsi) {
  (void)rsi;
  return sig_decision_call_recovery;
}

static union stdc_siginfo_value abort_guarded_fn(union stdc_siginfo_value v) {
  (void)v;
  LIBC_NAMESPACE::abort();
  return stdc_siginfo_value{-1}; // not reached
}

TEST(LlvmLibcWg14Chain, AbortReentryRecovers) {
  sigset_t abrt{};
  LIBC_NAMESPACE::sigemptyset(&abrt);
  LIBC_NAMESPACE::sigaddset(&abrt, SIGABRT);
  void *abrt_inst = LIBC_NAMESPACE::siginstall(&abrt);
  ASSERT_NE(abrt_inst, nullptr);

  recovery_ran = 0;
  union stdc_siginfo_value v =
      LIBC_NAMESPACE::sigguarded(&abrt, abort_guarded_fn, recovery_fn,
                                 recovery_decider, stdc_siginfo_value{0});
  EXPECT_TRUE(recovery_ran);
  EXPECT_EQ(static_cast<int>(v.int_value),
            static_cast<int>(SIGGUARDED_FAILURE_VALUE.int_value));
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(abrt_inst), 0);
}

} // namespace
