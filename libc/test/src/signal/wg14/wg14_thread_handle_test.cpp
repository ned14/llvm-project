//===-- wg14_signals thread-local/global handling (hermetic port) ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Hermetic port of thrd_signal_handle_test.c (thread-local sigguarded
/// recovery, global decider claim, concurrent destroy of a global decider
/// while a raise is in flight) and nested_decider_rsi_test.c (analysis.md
/// NSTR: a nested delivery during a frame decider must not corrupt the outer
/// decider's rsi). The nested-delivery trigger uses kill(getpid(), signo)
/// instead of pthread_kill (libc does not provide a pthread_kill entrypoint);
/// the process is single-threaded in that section, so the signal is
/// delivered to the calling thread.
///
//===----------------------------------------------------------------------===//

#include "hdr/errno_macros.h"
#include "hdr/signal_macros.h"
#include "hdr/types/sigset_t.h"
#include "hdr/types/struct_stdc_siginfo.h"
#include "hdr/types/union_stdc_siginfo_value.h"
#include "src/signal/kill.h"
#include "src/signal/sigaddset.h"
#include "src/signal/sigemptyset.h"
#include "src/signal/sigguarded.h"
#include "src/signal/siginstall.h"
#include "src/signal/signal_decider_create.h"
#include "src/signal/signal_decider_destroy.h"
#include "src/signal/siguninstall.h"
#include "src/signal/stdc_raise.h"
#include "src/threads/thrd_create.h"
#include "src/threads/thrd_join.h"
#include "src/unistd/getpid.h"
#include "test/UnitTest/Test.h"

#include "src/__support/CPP/atomic.h"
#include <threads.h>

namespace {

#define SIGNAL_TO_USE SIGILL

struct shared_t {
  int count_decider, count_recovery;
  LIBC_NAMESPACE::cpp::Atomic<int> latch;
};

static union stdc_siginfo_value
sigill_recovery_func(const struct stdc_siginfo *rsi) {
  struct shared_t *shared =
      static_cast<struct shared_t *>(rsi->value.ptr_value);
  shared->count_recovery++;
  return rsi->value;
}

static enum sig_decision sigill_decider_func(struct stdc_siginfo *rsi) {
  struct shared_t *shared =
      static_cast<struct shared_t *>(rsi->value.ptr_value);
  shared->count_decider++;
  if (shared->latch.load(LIBC_NAMESPACE::cpp::MemoryOrder::ACQUIRE) == 1) {
    // Wait here until the other thread destroys this decider.
    shared->latch.store(2, LIBC_NAMESPACE::cpp::MemoryOrder::RELEASE);
    while (shared->latch.load(LIBC_NAMESPACE::cpp::MemoryOrder::ACQUIRE) == 2) {
    }
  }
  return sig_decision_call_recovery; // handled
}

static union stdc_siginfo_value sigill_func(union stdc_siginfo_value value) {
  (void)LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr);
  return value;
}

static int sigill_thread(void *arg) {
  (void)arg;
  return LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr);
}

// Thread-local sigguarded recovery, global decider claim, and concurrent
// destroy of a global decider whilst a raise is in flight.
TEST(LlvmLibcWg14ThreadHandle, ThreadLocalGlobalAndConcurrentDestroy) {
  void *handlers = LIBC_NAMESPACE::siginstall(nullptr);
  ASSERT_NE(handlers, nullptr);

  // Thread-local handling: a frame decider claiming call_recovery runs the
  // frame's recovery function.
  {
    struct shared_t shared{};
    union stdc_siginfo_value value;
    value.ptr_value = &shared;
    sigset_t guarded{};
    LIBC_NAMESPACE::sigemptyset(&guarded);
    LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
    (void)LIBC_NAMESPACE::sigguarded(&guarded, sigill_func,
                                     sigill_recovery_func, sigill_decider_func,
                                     value);
    EXPECT_EQ(shared.count_decider, 1);
    EXPECT_EQ(shared.count_recovery, 1);
  }

  // Global handling: a global decider claims the raise; no recovery function
  // is involved in the global path.
  {
    struct shared_t shared{};
    union stdc_siginfo_value value;
    value.ptr_value = &shared;
    sigset_t guarded{};
    LIBC_NAMESPACE::sigemptyset(&guarded);
    LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
    void *sigill_decider = LIBC_NAMESPACE::signal_decider_create(
        &guarded, false, sigill_decider_func, value);
    ASSERT_NE(sigill_decider, nullptr);
    EXPECT_TRUE(LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr));
    EXPECT_EQ(shared.count_decider, 1);
    EXPECT_EQ(shared.count_recovery, 0);
    EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(sigill_decider), 0);
  }

  // Concurrent destroy of a global decider whilst the raise is inside the
  // decider function: the raise must still complete and report success, and
  // the decider must run exactly once.
  {
    struct shared_t shared{};
    shared.latch.store(1, LIBC_NAMESPACE::cpp::MemoryOrder::RELAXED);
    union stdc_siginfo_value value;
    value.ptr_value = &shared;
    sigset_t guarded{};
    LIBC_NAMESPACE::sigemptyset(&guarded);
    LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
    void *sigill_decider = LIBC_NAMESPACE::signal_decider_create(
        &guarded, false, sigill_decider_func, value);
    ASSERT_NE(sigill_decider, nullptr);

    thrd_t th;
    ASSERT_EQ(LIBC_NAMESPACE::thrd_create(&th, sigill_thread, nullptr),
              int(thrd_success));
    while (shared.latch.load(LIBC_NAMESPACE::cpp::MemoryOrder::ACQUIRE) != 2) {
    }
    EXPECT_EQ(LIBC_NAMESPACE::signal_decider_destroy(sigill_decider), 0);
    shared.latch.store(0, LIBC_NAMESPACE::cpp::MemoryOrder::RELEASE);
    int res = 0;
    ASSERT_EQ(LIBC_NAMESPACE::thrd_join(th, &res), int(thrd_success));
    EXPECT_EQ(res, 1);
    EXPECT_EQ(shared.count_decider, 1);
    EXPECT_EQ(shared.count_recovery, 0);
  }

  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);
}

// Nested delivery during a frame decider must not corrupt the outer rsi
// (NSTR): each raise gets its own rsi; the outer raise passed NULL info, so
// after the nested real delivery the outer decider's raw_info must still be
// NULL, and the nested decider must see the kernel siginfo of its own
// delivery.
static LIBC_NAMESPACE::cpp::Atomic<int> outer_decider_calls{0};
static LIBC_NAMESPACE::cpp::Atomic<int> nested_decider_calls{0};
static volatile int outer_saw_nested_info = 0;
static volatile bool nested_saw_bad_info = false;

static enum sig_decision nested_rsi_decider(struct stdc_siginfo *rsi) {
  if (outer_decider_calls.fetch_add(1) == 0) {
    // First (outer) activation: software raise with NULL info.
    if (rsi->raw_info != nullptr)
      outer_saw_nested_info = 2;
    // Re-enter the same frame's decider via a real signal delivery.
    // SA_NODEFER leaves the signal unblocked inside the handler, so the
    // nested delivery interrupts this call; the kernel siginfo is what the
    // nested raise's prepare_rsi used to write into a shared frame->rsi.
    (void)LIBC_NAMESPACE::kill(LIBC_NAMESPACE::getpid(), SIGNAL_TO_USE);
    if (rsi->raw_info != nullptr)
      outer_saw_nested_info = 1;
    return sig_decision_resume_execution;
  }
  // Nested activation: real signal delivery, kernel siginfo must be present.
  nested_decider_calls.fetch_add(1);
  if (rsi->raw_info == nullptr || rsi->signo != SIGNAL_TO_USE)
    nested_saw_bad_info = 1;
  return sig_decision_resume_execution;
}

static union stdc_siginfo_value
raise_inside_guard(union stdc_siginfo_value value) {
  (void)LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr);
  return value;
}

static union stdc_siginfo_value noop_recovery(const struct stdc_siginfo *rsi) {
  return rsi->value;
}

TEST(LlvmLibcWg14ThreadHandle, NestedDeliveryKeepsOwnRsi) {
  sigset_t guarded{};
  LIBC_NAMESPACE::sigemptyset(&guarded);
  LIBC_NAMESPACE::sigaddset(&guarded, SIGNAL_TO_USE);
  void *handlers = LIBC_NAMESPACE::siginstall(&guarded);
  ASSERT_NE(handlers, nullptr);

  outer_decider_calls.store(0);
  nested_decider_calls.store(0);
  outer_saw_nested_info = 0;
  nested_saw_bad_info = 0;
  union stdc_siginfo_value raise_value{7};
  union stdc_siginfo_value ret_value =
      LIBC_NAMESPACE::sigguarded(&guarded, raise_inside_guard, noop_recovery,
                                 nested_rsi_decider, raise_value);
  // Both activations ran: the outer software raise and the nested real
  // delivery.
  EXPECT_EQ(outer_decider_calls.load(), 2);
  EXPECT_EQ(nested_decider_calls.load(), 1);
  // The nested decider saw the kernel siginfo of its own delivery.
  EXPECT_FALSE(nested_saw_bad_info);
  // The outer decider's rsi survived the nested delivery unclobbered.
  EXPECT_EQ(outer_saw_nested_info, 0);
  EXPECT_EQ(static_cast<int>(ret_value.int_value), 7);
  EXPECT_EQ(LIBC_NAMESPACE::siguninstall(handlers), 0);

  // After uninstall the frame is popped and the map entry gone: an unclaimed
  // raise returns false instead of re-raising SIGILL at the default action.
  EXPECT_FALSE(LIBC_NAMESPACE::stdc_raise(SIGNAL_TO_USE, nullptr, nullptr));
  EXPECT_EQ(outer_decider_calls.load(), 2);
}

} // namespace
