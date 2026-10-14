//===-- Definition of struct tss_async_signal_safe_attr -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definition of the tss_async_signal_safe_attr struct (WG14 N3924 improved
/// C signals). Mirrors
/// libc/src/signal/wg14/include/wg14_signals/tss_async_signal_safe.h.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_STRUCT_TSS_ASYNC_SIGNAL_SAFE_ATTR_H
#define LLVM_LIBC_TYPES_STRUCT_TSS_ASYNC_SIGNAL_SAFE_ATTR_H

// The attributes for creating an async signal safe thread local.
struct tss_async_signal_safe_attr {
  int (*const create)(void **dest); // Create an instance
  int (*const destroy)(void *v);    // Destroy an instance
};

#endif // LLVM_LIBC_TYPES_STRUCT_TSS_ASYNC_SIGNAL_SAFE_ATTR_H
