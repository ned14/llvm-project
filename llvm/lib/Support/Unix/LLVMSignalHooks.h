//===- LLVMSignalHooks.h - wg14_signals embedder hooks ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Embedder override hooks for the wg14_signals library (the WG14 N3924
// improved-signals reference implementation, vendored as the
// libc/src/signal/wg14 git submodule) when compiled into LLVM.
//
// This header is force-included (-include) into every wg14_signals library
// source by the LLVMwg14_signals object library, so the hooks apply to the
// compiled library; Unix/Signals.inc includes it too so the declarations
// match.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_SUPPORT_UNIX_LLVMSIGNALHOOKS_H
#define LLVM_LIB_SUPPORT_UNIX_LLVMSIGNALHOOKS_H

#include <signal.h>

// The library's raw signal handler must run on LLVM's alternate signal
// stack (created by CreateSigAltStack) so that stack-overflow SIGSEGV
// handling keeps working; LLVM requests this at runtime via
// siginstall_set_sa_flags_np() in RegisterHandlersThreadsafe()
// (Unix/Signals.inc) before the first siginstall().

// Take a signal's default action, preserving the original siginfo so that
// the core dump records the faulting address. Defined in Unix/Signals.inc;
// registered at runtime via siginstall_set_default_action_np() in
// RegisterHandlersThreadsafe() (Unix/Signals.inc) before the first
// siginstall().
#ifdef __cplusplus
extern "C" {
#endif
void LLVMSignalDefaultAction(int Signo, siginfo_t *Info, void *Context);
#ifdef __cplusplus
}
#endif

#endif
