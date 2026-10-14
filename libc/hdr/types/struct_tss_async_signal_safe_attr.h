//===-- Proxy for struct_tss_async_signal_safe_attr -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIBC_HDR_TYPES_STRUCT_TSS_ASYNC_SIGNAL_SAFE_ATTR_H
#define LLVM_LIBC_HDR_TYPES_STRUCT_TSS_ASYNC_SIGNAL_SAFE_ATTR_H

#ifdef LIBC_FULL_BUILD

#include "include/llvm-libc-types/struct_tss_async_signal_safe_attr.h"

#else

#include <signal.h>
#include <threads.h>

#endif // LIBC_FULL_BUILD

#endif // LLVM_LIBC_HDR_TYPES_STRUCT_TSS_ASYNC_SIGNAL_SAFE_ATTR_H
