//===--  -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Internal header for the WG14 N3924 current_thread_id entrypoint
/// (implemented by the wg14_signals reference implementation submodule,
/// libc/src/signal/wg14). Placed in <threads.h> per the N3924 rev 5 wording.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_THREADS_CURRENT_THREAD_ID_H
#define LLVM_LIBC_SRC_THREADS_CURRENT_THREAD_ID_H

#include "hdr/types/thread_id_t.h"
#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {

thread_id_t current_thread_id(void);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_THREADS_CURRENT_THREAD_ID_H
