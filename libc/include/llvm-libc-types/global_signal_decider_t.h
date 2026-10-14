//===-- Definition of type global_signal_decider_t ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Opaque definition of the global_signal_decider_t type (WG14 N3924
/// improved C signals). Deciders are created and destroyed via
/// signal_decider_create()/signal_decider_destroy(); the internals are
/// implementation defined.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_GLOBAL_SIGNAL_DECIDER_T_H
#define LLVM_LIBC_TYPES_GLOBAL_SIGNAL_DECIDER_T_H

typedef struct global_signal_decider_t global_signal_decider_t;

#endif // LLVM_LIBC_TYPES_GLOBAL_SIGNAL_DECIDER_T_H
