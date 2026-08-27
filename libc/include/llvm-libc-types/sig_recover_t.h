//===-- Definition of sig_recover_t type ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the sig_recover_t type (WG14 N3924 improved C signals).
/// Mirrors libc/src/signal/wg14/include/wg14_signals/thrd_signal_handle.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_SIG_RECOVER_T_H
#define LLVM_LIBC_TYPES_SIG_RECOVER_T_H

#include "struct_stdc_siginfo.h"
#include "union_stdc_siginfo_value.h"

// The type of the function called to recover from a signal being raised in
// a guarded section.
typedef union stdc_siginfo_value(sig_recover_t)(const struct stdc_siginfo *);

#endif // LLVM_LIBC_TYPES_SIG_RECOVER_T_H
