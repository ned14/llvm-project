#!/usr/bin/env bash
#===----------------------------------------------------------------------===##
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===##

#
# Installs the compiler toolchain used by the libc++ pre-commit workflows on
# GitHub-hosted runners. The LLVM self-hosted runners already ship these
# compilers, so this script is a no-op when they are present (which keeps the
# upstream behaviour unchanged).
#

set -euxo pipefail

if ! command -v clang-23 >/dev/null 2>&1; then
  wget -q https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh 23
fi
