//===-- Definition of sig_decide_t type -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the sig_decide_t type (WG14 N3924 improved C signals).
/// Mirrors libc/src/signal/wg14/include/wg14_signals/thrd_signal_handle.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_SIG_DECIDE_T_H
#define LLVM_LIBC_TYPES_SIG_DECIDE_T_H

#include "sig_decision.h"
#include "struct_stdc_siginfo.h"

// The type of the function called when a signal is raised. Returns a
// decision of how to handle the signal.
typedef enum sig_decision(sig_decide_t)(struct stdc_siginfo *);

#endif // LLVM_LIBC_TYPES_SIG_DECIDE_T_H
