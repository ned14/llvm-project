//===-- Proxy for struct_stdc_siginfo ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIBC_HDR_TYPES_STRUCT_STDC_SIGINFO_H
#define LLVM_LIBC_HDR_TYPES_STRUCT_STDC_SIGINFO_H

#ifdef LIBC_FULL_BUILD

#include "include/llvm-libc-types/struct_stdc_siginfo.h"

#else

#include <signal.h>
#include <threads.h>

#endif // LIBC_FULL_BUILD

#endif // LLVM_LIBC_HDR_TYPES_STRUCT_STDC_SIGINFO_H
