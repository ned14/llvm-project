//===--  -------------------------------------------*- C++ -*-===//
//-----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Internal header for the WG14 N3924 tss_async_signal_safe_create entrypoint
/// (implemented by the wg14_signals reference implementation submodule,
/// libc/src/signal/wg14).
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_SIGNAL_TSS_ASYNC_SIGNAL_SAFE_CREATE_H
#define LLVM_LIBC_SRC_SIGNAL_TSS_ASYNC_SIGNAL_SAFE_CREATE_H

#include "hdr/types/struct_tss_async_signal_safe_attr.h"
#include "hdr/types/tss_async_signal_safe_t.h"
#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {

int tss_async_signal_safe_create(tss_async_signal_safe_t *val,
                                 const struct tss_async_signal_safe_attr *attr);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_SIGNAL_TSS_ASYNC_SIGNAL_SAFE_CREATE_H
