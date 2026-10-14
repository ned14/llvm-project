//===-- Definition of type thread_id_t ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the thread_id_t type (WG14 N3924 improved C signals).
/// Mirrors libc/src/signal/wg14/include/wg14_signals/current_thread_id.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_THREAD_ID_T_H
#define LLVM_LIBC_TYPES_THREAD_ID_T_H

#include "../llvm-libc-macros/stdint-macros.h"

typedef uintptr_t thread_id_t;

#endif // LLVM_LIBC_TYPES_THREAD_ID_T_H
