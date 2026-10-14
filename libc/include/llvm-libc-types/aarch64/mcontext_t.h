//===-- Definition of type mcontext_t -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Note: Definitions in this file are based on the Linux kernel ABI.

#ifndef LLVM_LIBC_TYPES_AARCH64_MCONTEXT_T_H
#define LLVM_LIBC_TYPES_AARCH64_MCONTEXT_T_H

// This definition matches the kernel's 'struct sigcontext' and glibc's
// aarch64 mcontext_t (see arch/arm64/include/uapi/asm/sigcontext.h and
// glibc's sysdeps/unix/sysv/linux/aarch64/sys/ucontext.h).
typedef struct {
  unsigned long long fault_address;
  unsigned long long regs[31];
  unsigned long long sp;
  unsigned long long pc;
  unsigned long long pstate;
  // This field contains extension records for additional processor state
  // such as the FP/SIMD state. It has to match the definition of the
  // corresponding field in the sigcontext struct.
  unsigned char __reserved[4096] __attribute__((__aligned__(16)));
} mcontext_t;

#endif // LLVM_LIBC_TYPES_AARCH64_MCONTEXT_T_H
