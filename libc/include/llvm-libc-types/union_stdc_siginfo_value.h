//===-- Definition of union stdc_siginfo_value ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the stdc_siginfo_value union (WG14 N3924 improved C
/// signals). The layout mirrors the wg14_signals reference implementation
/// (libc/src/signal/wg14/include/wg14_signals/thrd_signal_handle.h); keep
/// the two in sync.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_STDC_SIGINFO_VALUE_H
#define LLVM_LIBC_TYPES_STDC_SIGINFO_VALUE_H

#include "../llvm-libc-macros/stdint-macros.h"

union stdc_siginfo_value {
  intptr_t int_value;
  void *ptr_value;
};

// A value whose int_value member is -99, returned by sigguarded() if it
// fails to install the guard (N3924 7.14.2.1). C: an object-like macro
// expanding to a compound literal. C++: an inline constexpr variable.
#ifdef __cplusplus
inline constexpr union stdc_siginfo_value SIGGUARDED_FAILURE_VALUE = {-99};
#else
#define SIGGUARDED_FAILURE_VALUE ((union stdc_siginfo_value){.int_value = -99})
#endif

#endif // LLVM_LIBC_TYPES_STDC_SIGINFO_VALUE_H
