//===-- WG14 N3924 sigfillset_asynchronous_debug entrypoint wrapper
//-----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// Thin wrapper over the wg14_signals reference implementation (the pristine
// submodule at libc/src/signal/wg14). The submodule's symbols are compiled
// with WG14_SIGNALS_PREFIX(x)=wg14_signals_##x; the prefixed prototypes are
// declared here with the equivalent unprefixed libc types (identical
// layouts, C linkage, so the ABI matches), keeping this TU free of the
// submodule's own header and its macro/symbol collisions with the generated
// <signal.h>.
//===----------------------------------------------------------------------===//

#include "src/signal/sigfillset_asynchronous_debug.h"
#include "src/__support/common.h"
#include "src/__support/macros/config.h"

extern "C"
{

  int wg14_signals_sigfillset_asynchronous_debug(sigset_t *set);
}

namespace LIBC_NAMESPACE_DECL
{

  LLVM_LIBC_FUNCTION(int, sigfillset_asynchronous_debug, (sigset_t * set))
  {
    return wg14_signals_sigfillset_asynchronous_debug(set);
  }

}  // namespace LIBC_NAMESPACE_DECL
