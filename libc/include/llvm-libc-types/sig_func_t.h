//===-- Definition of sig_func_t type -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the sig_func_t type (WG14 N3924 improved C signals).
/// Mirrors libc/src/signal/wg14/include/wg14_signals/thrd_signal_handle.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_SIG_FUNC_T_H
#define LLVM_LIBC_TYPES_SIG_FUNC_T_H

#include "union_stdc_siginfo_value.h"

// The type of the guarded function.
typedef union stdc_siginfo_value(sig_func_t)(union stdc_siginfo_value);

#endif // LLVM_LIBC_TYPES_SIG_FUNC_T_H
