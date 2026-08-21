//===-- Definition of type tss_async_signal_safe_t ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Opaque definition of the tss_async_signal_safe_t type (WG14 N3924
/// improved C signals). Mirrors
/// libc/src/signal/wg14/include/wg14_signals/tss_async_signal_safe.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_TSS_ASYNC_SIGNAL_SAFE_T_H
#define LLVM_LIBC_TYPES_TSS_ASYNC_SIGNAL_SAFE_T_H

typedef struct tss_async_signal_safe_s *tss_async_signal_safe_t;

#endif // LLVM_LIBC_TYPES_TSS_ASYNC_SIGNAL_SAFE_T_H
