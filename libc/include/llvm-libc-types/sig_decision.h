//===-- Definition of enum sig_decision -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the sig_decision enum (WG14 N3924 improved C signals).
/// Mirrors libc/src/signal/wg14/include/wg14_signals/thrd_signal_handle.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_SIG_DECISION_H
#define LLVM_LIBC_TYPES_SIG_DECISION_H

enum sig_decision {
  // We have decided to do nothing
  sig_decision_next_decider,
  // We have fixed the cause of the signal, please resume execution
  sig_decision_resume_execution,
  // Thread local signal deciders only: reset the stack and local state to
  // entry to sigguarded(), and call the recovery function.
  sig_decision_call_recovery
};

#endif // LLVM_LIBC_TYPES_SIG_DECISION_H
