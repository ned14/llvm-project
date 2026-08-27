//===-- WG14 N3924 siginstall entrypoint wrapper -----------------*- C++
//-*-===//
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

#include "src/signal/siginstall.h"
#include "src/__support/common.h"
#include "src/__support/macros/config.h"

extern "C"
{

  void *wg14_signals_siginstall(const sigset_t *guarded);
}

namespace LIBC_NAMESPACE_DECL
{

  LLVM_LIBC_FUNCTION(void *, siginstall, (const sigset_t *guarded))
  {
    return wg14_signals_siginstall(guarded);
  }

}  // namespace LIBC_NAMESPACE_DECL
