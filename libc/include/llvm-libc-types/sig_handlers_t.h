//===-- Definition of sig_handlers_t type --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the sig_handlers_t type (WG14 N3924 improved C signals).
/// Mirrors libc/src/signal/wg14/include/wg14_signals/thrd_signal_handle.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_SIG_HANDLERS_T_H
#define LLVM_LIBC_TYPES_SIG_HANDLERS_T_H

// An implementation-defined opaque pointer type that represents a set of
// installed signal handlers (7.14.1). It is the type of the value returned by
// siginstall() and of the argument of siguninstall(); a null pointer value
// represents no installation.
typedef struct sig_handlers_s *sig_handlers_t;

#endif // LLVM_LIBC_TYPES_SIG_HANDLERS_T_H
