//===--  -------------------------------------------*- C++ -*-===//
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Internal header for the WG14 N3924 stdc_raise entrypoint (implemented by the
/// wg14_signals reference implementation submodule,
/// libc/src/signal/wg14).
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_SIGNAL_STDC_RAISE_H
#define LLVM_LIBC_SRC_SIGNAL_STDC_RAISE_H

#include "hdr/types/sig_decide_t.h"
#include "hdr/types/sig_func_t.h"
#include "hdr/types/sig_recover_t.h"
#include "hdr/types/sigset_t.h"
#include "hdr/types/struct_stdc_siginfo.h"
#include "hdr/types/union_stdc_siginfo_value.h"
#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {

bool stdc_raise(int signo, void *raw_info, void *raw_context);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_SIGNAL_STDC_RAISE_H
