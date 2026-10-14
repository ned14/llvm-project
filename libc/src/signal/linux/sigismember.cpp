//===-- Linux implementation of sigismember ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/signal/sigismember.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"
#include "src/__support/macros/config.h"
#include "src/signal/linux/signal_utils.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, sigismember, (const sigset_t *set, int signum)) {
  if (!set) {
    libc_errno = EINVAL;
    return -1;
  }
  if (signum <= 0 || signum >= NSIG) {
    libc_errno = EINVAL;
    return -1;
  }
  size_t n = size_t(signum) - 1;
  size_t word = n / BITS_PER_SIGWORD;
  size_t bit = n % BITS_PER_SIGWORD;
  return (set->__signals[word] & (1UL << bit)) != 0;
}

} // namespace LIBC_NAMESPACE_DECL
