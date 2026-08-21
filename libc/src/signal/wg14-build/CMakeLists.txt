#===-- wg14_signals integration build -------------------------*- CMake -*-===#
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
#
# The WG14 N3924 improved C signals reference implementation, mounted as a
# pristine git submodule (https://github.com/ned14/wg14_signals.git,
# Apache-2.0) at libc/src/signal/wg14. libc's build consumes the submodule's
# C sources directly under the full-build flag set and never patches them:
# the recursion-avoidance shim is the embedder override layer upstreamed in
# the submodule's config.h (WG14_SIGNALS_SIGACTION/ABORT/KILL_SELF/GETTID/
# MEMCPY/MEMSET/MALLOC/CALLOC/FREE/SIG*SET/PTHREAD_*/SETJMP/LONGJMP), which
# embedder_shim.{h,cpp} below redefines to libc's own kernel-facing and
# internal layers.
#
# IMPORTANT: git cannot track files inside a submodule directory (the
# directory is a gitlink), so ALL fork-owned files live in this sibling
# directory libc/src/signal/wg14-build, never inside libc/src/signal/wg14.
# The submodule is pristine and its own CMakeLists.txt (a standalone
# project()) is untouched; it must NOT be add_subdirectory()'d here.
#
# Bumping the integration is a deliberate `git submodule` commit; the
# submodule's own test suite (libc/src/signal/wg14/test) is its primary
# correctness net.
#
#===----------------------------------------------------------------------===#

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../wg14/include/wg14_signals/config.h)
  message(FATAL_ERROR
    "The wg14_signals submodule is missing at libc/src/signal/wg14.\n"
    "Run: git submodule update --init libc/src/signal/wg14\n"
    "then reconfigure this build.")
endif()

set(wg14_compile_options
  # The submodule's public symbols are prefixed so they cannot collide
  # with the unprefixed libc entrypoints (both live in libc.a); the
  # entrypoint wrappers use the same prefix.
  "-DWG14_SIGNALS_PREFIX(x)=wg14_signals_##x"
  # libc's generated <signal.h> provides its own sigfence macro and
  # SIGGUARDED_FAILURE_VALUE mirrors; suppress the submodule's copies so
  # there is no macro/duplicate-symbol redefinition in TUs that see both
  # headers (see the embedder guards in the submodule's config.h and
  # thrd_signal_handle.h).
  -DWG14_SIGNALS_DISABLE_SIGFENCE_MACRO
  -DWG14_SIGNALS_DISABLE_SIGGUARDED_FAILURE_VALUE
  # The submodule compiles clean under -Wall -Wextra -Wpedantic (its own
  # build's flags); libc's -Wconversion/-Wcast-qual/-Wglobal-constructors
  # trip only on the vendored verstable.h hash table (intentional double
  # load-factor math and a const-cast placeholder) and the lazy-init
  # __attribute__((constructor)) sigset builders, so relax those for the
  # submodule.
  -Wno-conversion
  -Wno-cast-qual
  -Wno-global-constructors
  -include;${CMAKE_CURRENT_SOURCE_DIR}/embedder_shim.h
  -I${CMAKE_CURRENT_SOURCE_DIR}/../wg14/include
)

# The recursion-free embedder shim (raw-syscall sigaction/abort/kill_self/
# gettid). Its own object library so any TU that references the hooks (the
# wg14 sources, and any TU referencing the embedder hooks) pulls the objects
# in.
add_object_library(
  embedder_shim
  SRCS
    embedder_shim.cpp
  HDRS
    embedder_shim.h
  DEPENDS
    libc.include.signal
    libc.include.sys_syscall
    libc.src.__support.OSUtil.osutil
    libc.src.__support.macros.config
)

# The host-call bridge: extern "C" implementations of the embedder hooks
# for memory/sigset/setjmp/pthread/allocator calls, routing to libc's
# internal C++ entrypoints (which exist in both the library and the
# hermetic-test object worlds). Separate object so its own references
# (the allocator C symbols) only matter when the wg14 objects are pulled.
add_object_library(
  wg14_hostcalls
  SRCS
    wg14_hostcalls.cpp
  HDRS
    embedder_shim.h
  DEPENDS
    libc.src.pthread.pthread_getspecific
    libc.src.pthread.pthread_key_create
    libc.src.pthread.pthread_once
    libc.src.pthread.pthread_setspecific
    libc.src.setjmp.longjmp
    libc.src.setjmp.setjmp
    libc.src.stdio.stderr
    libc.src.stdio.vfprintf
    libc.src.signal.sigaddset
    libc.src.signal.sigdelset
    libc.src.signal.sigemptyset
    libc.src.signal.sigfillset
    libc.src.signal.sigismember
    libc.src.string.memcpy
    libc.src.string.memset
    libc.include.signal
    libc.include.setjmp
    libc.src.__support.macros.config
)

add_object_library(
  wg14_signals
  SRCS
    ../wg14/src/wg14_signals/current_thread_id.c
    ../wg14/src/wg14_signals/thread_atexit.c
    ../wg14/src/wg14_signals/thrd_signal_handle_posix.c
    ../wg14/src/wg14_signals/tss_async_signal_safe.c
  HDRS
    ../wg14/include/wg14_signals/config.h
    ../wg14/include/wg14_signals/current_thread_id.h
    ../wg14/include/wg14_signals/thrd_signal_handle.h
    ../wg14/include/wg14_signals/tss_async_signal_safe.h
    ../wg14/include/wg14_signals/detail/impl/current_thread_id.c.ipp
    ../wg14/include/wg14_signals/detail/impl/linked_list.h
    ../wg14/include/wg14_signals/detail/impl/lock_unlock.h
    ../wg14/include/wg14_signals/detail/impl/thrd_signal_handle_common.ipp.ipp
    ../wg14/include/wg14_signals/detail/impl/thrd_signal_handle_posix.c.ipp
    ../wg14/include/wg14_signals/detail/impl/thrd_signal_handle_windows.c.ipp
    ../wg14/include/wg14_signals/detail/impl/thread_atexit.c.ipp
    ../wg14/include/wg14_signals/detail/impl/thread_atexit.cpp.ipp
    ../wg14/include/wg14_signals/detail/impl/thread_atexit.h
    ../wg14/include/wg14_signals/detail/impl/tss_async_signal_safe.c.ipp
    ../wg14/include/wg14_signals/detail/impl/verstable.h
  DEPENDS
    .wg14_hostcalls
    .embedder_shim
    libc.include.assert
    libc.include.errno
    libc.include.pthread
    libc.include.setjmp
    libc.include.signal
    libc.include.stdint
    libc.include.stdio
    libc.include.stdlib
    libc.include.string
    libc.include.sys_syscall
    libc.include.ucontext
    libc.include.unistd
  COMPILE_OPTIONS
    # The submodule sources are C11; the entrypoint wrappers are C++ and get
    # their standard from add_entrypoint_object.
    -std=gnu11
    ${wg14_compile_options}
)

# The wg14 code allocates (siginstall/signal_decider_create/tss_*); when
# scudo is the allocator, tests that pull the wg14 objects need the scudo
# runtime in their link. Missing RTScudo targets (scudo OFF) are skipped by
# the test rules. The malloc/calloc/free/realloc *entrypoint* targets are
# UTILITY externals and cannot be linked into object libraries, so the
# scudo libraries are named directly.
if(LLVM_LIBC_INCLUDE_SCUDO)
  # The scudo targets are named after the libc target architecture on the
  # Linux/FreeBSD/darwin targets this submodule builds on.
  target_link_libraries(libc.src.signal.wg14-build.wg14_signals PUBLIC
    RTScudoStandalone.${LIBC_TARGET_ARCHITECTURE}
    RTScudoStandaloneCWrappers.${LIBC_TARGET_ARCHITECTURE})
endif()

add_subdirectory(entrypoints)
