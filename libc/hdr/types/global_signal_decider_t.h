//===-- Proxy for global_signal_decider_t -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIBC_HDR_TYPES_GLOBAL_SIGNAL_DECIDER_T_H
#define LLVM_LIBC_HDR_TYPES_GLOBAL_SIGNAL_DECIDER_T_H

#ifdef LIBC_FULL_BUILD

#include "include/llvm-libc-types/global_signal_decider_t.h"

#else

#include <signal.h>
#include <threads.h>

#endif // LIBC_FULL_BUILD

#endif // LLVM_LIBC_HDR_TYPES_GLOBAL_SIGNAL_DECIDER_T_H
