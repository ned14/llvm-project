//===-- wg14_signals tss_async_signal_safe (hermetic port) ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Hermetic port of tss_destroy_reentrancy_test.c (analysis.md UCLK):
/// tss_async_signal_safe_destroy must run the user's attr.destroy callbacks
/// WITHOUT holding the internal lock, so a documented-valid re-entrant
/// get() from the callback cannot self-deadlock, and must never return the
/// value whose destroy callback is running.
///
//===----------------------------------------------------------------------===//

#include "hdr/types/struct_tss_async_signal_safe_attr.h"
#include "hdr/types/tss_async_signal_safe_t.h"
#include "src/threads/thrd_create.h"
#include "src/threads/thrd_join.h"
#include "src/threads/tss_async_signal_safe_create.h"
#include "src/threads/tss_async_signal_safe_destroy.h"
#include "src/threads/tss_async_signal_safe_get.h"
#include "src/threads/tss_async_signal_safe_thread_init.h"
#include "test/UnitTest/Test.h"

#include "src/__support/CPP/atomic.h"
#include <stdlib.h>
#include <threads.h>

namespace {

static tss_async_signal_safe_t g_tls;
static int destroy_calls = 0;
static bool get_never_returned_being_destroyed = true;

static int create_cb(void **dest) {
  unsigned *v = static_cast<unsigned *>(malloc(sizeof(unsigned)));
  if (v == nullptr)
    return -1;
  *v = 42;
  *dest = v;
  return 0;
}

static int destroy_cb(void *v) {
  // Re-entrant get() on the same handle mid-destroy: must not deadlock, and
  // must never return the value whose destroy callback is running (that
  // value was erased from the map before its callback ran, so get() can only
  // see NULL or a still-live sibling whose own callback has not run yet).
  if (LIBC_NAMESPACE::tss_async_signal_safe_get(g_tls) == v)
    get_never_returned_being_destroyed = 0;
  destroy_calls++;
  free(v);
  return 0;
}

static LIBC_NAMESPACE::cpp::Atomic<int> worker_ready;
static LIBC_NAMESPACE::cpp::Atomic<int> destroy_done;

static int worker_thr(void *unused) {
  (void)unused;
  const int init_ret = LIBC_NAMESPACE::tss_async_signal_safe_thread_init(g_tls);
  worker_ready.store(1, LIBC_NAMESPACE::cpp::MemoryOrder::RELEASE);
  if (init_ret != 0)
    return -1;
  // Stay registered (do not run the exit-time deinit, which would erase this
  // thread's map entry) until the main thread has destroyed the handle, so
  // destroy() iterates two live entries.
  while (destroy_done.load(LIBC_NAMESPACE::cpp::MemoryOrder::ACQUIRE) == 0) {
  }
  return 0;
}

// Basics plus destroy-callback re-entrancy: two live entries, each callback
// re-enters get() on the same handle (UCLK).
TEST(LlvmLibcWg14Tss, DestroyCallbackReentrancy) {
  worker_ready.store(0, LIBC_NAMESPACE::cpp::MemoryOrder::RELAXED);
  destroy_done.store(0, LIBC_NAMESPACE::cpp::MemoryOrder::RELAXED);
  struct tss_async_signal_safe_attr attr{create_cb, destroy_cb};
  EXPECT_EQ(LIBC_NAMESPACE::tss_async_signal_safe_create(&g_tls, &attr), 0);
  EXPECT_EQ(LIBC_NAMESPACE::tss_async_signal_safe_thread_init(g_tls), 0);
  unsigned *main_val =
      static_cast<unsigned *>(LIBC_NAMESPACE::tss_async_signal_safe_get(g_tls));
  ASSERT_NE(main_val, nullptr);
  EXPECT_EQ(static_cast<int>(*main_val), 42);

  thrd_t thr;
  int res = 0;
  ASSERT_EQ(LIBC_NAMESPACE::thrd_create(&thr, worker_thr, nullptr),
            int(thrd_success));
  // Wait until the worker's entry is registered: the worker's release store
  // of worker_ready happens-after its thread_init, so this acquire load
  // synchronises with it.
  while (worker_ready.load(LIBC_NAMESPACE::cpp::MemoryOrder::ACQUIRE) == 0) {
  }
  // Two entries are live; destroy runs both attr.destroy callbacks with the
  // lock released, and each callback re-enters get() on the same handle.
  destroy_calls = 0;
  get_never_returned_being_destroyed = 1;
  EXPECT_EQ(LIBC_NAMESPACE::tss_async_signal_safe_destroy(g_tls), 0);
  EXPECT_EQ(destroy_calls, 2);
  EXPECT_TRUE(get_never_returned_being_destroyed);
  destroy_done.store(1, LIBC_NAMESPACE::cpp::MemoryOrder::RELEASE);
  ASSERT_EQ(LIBC_NAMESPACE::thrd_join(thr, &res), int(thrd_success));
  EXPECT_EQ(res, 0);
}

} // namespace
