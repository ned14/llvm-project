//===-- Definition of struct stdc_siginfo ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the stdc_siginfo struct (WG14 N3924 improved C signals).
/// The layout mirrors the wg14_signals reference implementation
/// (libc/src/signal/wg14/include/wg14_signals/thrd_signal_handle.h); keep
/// the two in sync. The raw_info/raw_context members are pointers into OS
/// signal delivery data (siginfo_t / ucontext_t); they are spelled as void *
/// here so this header does not need to define ucontext_t, and the pointers
/// have identical layout.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_STDC_SIGINFO_H
#define LLVM_LIBC_TYPES_STDC_SIGINFO_H

#include <stdbool.h>

#include "union_stdc_siginfo_value.h"

struct sig_global_state_tss_state_per_frame_t;
struct sighandler_info;
struct global_signal_decider_t;

struct stdc_siginfo {
  int signo; // The signal raised

  // The system specific error code for this signal, the si_errno code
  // (POSIX). Zero when the raise carried no OS info (e.g.
  // stdc_raise(signo, NULL, NULL)).
  int error_code;
  void *addr; // Memory location which caused fault, if appropriate. NULL when
              // the raise carried no OS info.
  union stdc_siginfo_value value; // A user-defined value

  // The OS specific signal info; on POSIX a stdc_raise(signo, NULL, NULL)
  // sets raw_info to NULL.
  void *raw_info;
  // The OS specific ucontext_t; on POSIX passed through unchanged and may be
  // NULL.
  void *raw_context;

  // Used internally only
  struct sig_global_state_tss_state_per_frame_t *internal_local_decider;
  struct sighandler_info *internal_sighandler;
  struct global_signal_decider_t *internal_global_decider;
  bool internal_decider_is_abandoned;
};

#endif // LLVM_LIBC_TYPES_STDC_SIGINFO_H
