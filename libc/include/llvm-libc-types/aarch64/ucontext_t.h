//===-- Definition of type ucontext_t -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Note: Definitions in this file are based on the Linux kernel ABI.

#ifndef LLVM_LIBC_TYPES_AARCH64_UCONTEXT_T_H
#define LLVM_LIBC_TYPES_AARCH64_UCONTEXT_T_H

#include "../sigset_t.h"
#include "../stack_t.h"
#include "mcontext_t.h"

// The following fields must match the Linux kernel's struct ucontext on
// aarch64 (arch/arm64/include/uapi/asm/ucontext.h) to ensure ABI
// compatibility for signal handling: libc's sigset_t is the kernel-sized
// 8-byte set, so the kernel's 120-byte pad between uc_sigmask and
// uc_mcontext appears explicitly (glibc hides it inside its larger
// __sigset_t; the offsets of uc_mcontext are identical).
typedef struct ucontext_t {
  unsigned long uc_flags;
  struct ucontext_t *uc_link;
  stack_t uc_stack;
  sigset_t uc_sigmask;
  unsigned char __reserved[1024 / 8 - (sizeof(sigset_t) / 8)];
  mcontext_t uc_mcontext;
} ucontext_t;

#endif // LLVM_LIBC_TYPES_AARCH64_UCONTEXT_T_H
