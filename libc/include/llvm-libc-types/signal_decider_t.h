//===-- Definition of signal_decider_t type ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the signal_decider_t type (WG14 N3924 improved C signals).
/// Mirrors libc/src/signal/wg14/include/wg14_signals/thrd_signal_handle.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_SIGNAL_DECIDER_T_H
#define LLVM_LIBC_TYPES_SIGNAL_DECIDER_T_H

// An implementation-defined opaque pointer type that represents an installed
// global signal decider (7.14.1). It is the type of the value returned by
// signal_decider_create() and of the argument of signal_decider_destroy(); a
// null pointer value represents no decider.
typedef struct signal_decider_s *signal_decider_t;

#endif // LLVM_LIBC_TYPES_SIGNAL_DECIDER_T_H
