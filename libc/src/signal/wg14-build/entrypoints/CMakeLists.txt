#===-- wg14_signals N3924 entrypoint wrappers ------------------*- CMake -*-===#
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#
#
# Thin LLVM_LIBC_FUNCTION wrappers over the wg14_signals reference
# implementation (the pristine submodule at libc/src/signal/wg14). The
# submodule's public symbols are compiled with WG14_SIGNALS_PREFIX set to
# wg14_signals_ so they cannot collide with the unprefixed libc entrypoints
# (both live in libc.a); the wrapper TUs declare the prefixed prototypes
# with the equivalent unprefixed libc types (identical layouts, C linkage)
# and never include the submodule's own header, so the generated <signal.h>
# and the submodule header cannot collide either.
#
#===----------------------------------------------------------------------===#

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../wg14/include/wg14_signals/config.h)
  message(FATAL_ERROR
    "The wg14_signals submodule is missing at libc/src/signal/wg14.\n"
    "Run: git submodule update --init libc/src/signal/wg14\n"
    "then reconfigure this build.")
endif()

function(add_wg14_entrypoint name)
  # Functions declared in <threads.h> (the tss_async_signal_safe_* family,
  # N3924 7.30.6.5-8, and current_thread_id) have their internal headers in
  # src/threads; the signal.h functions' headers are in src/signal.
  if(name STREQUAL "current_thread_id")
    set(entry_header ../../../threads/current_thread_id.h)
  elseif(name MATCHES "^tss_async_signal_safe_")
    set(entry_header ../../../threads/${name}.h)
  else()
    set(entry_header ../../${name}.h)
  endif()
  add_entrypoint_object(
    ${name}
    SRCS
      ${name}.cpp
    HDRS
      ${entry_header}
    DEPENDS
      libc.src.signal.wg14-build.wg14_signals
      libc.include.signal
      libc.hdr.types.sigset_t
      libc.hdr.types.struct_stdc_siginfo
      libc.hdr.types.union_stdc_siginfo_value
  )
endfunction()

add_wg14_entrypoint(sigfillset_synchronous)
add_wg14_entrypoint(sigfillset_asynchronous_nondebug)
add_wg14_entrypoint(sigfillset_asynchronous_debug)
add_wg14_entrypoint(sigguarded)
add_wg14_entrypoint(sigdecider_abandon)
add_wg14_entrypoint(sigdecider_abandon_resume)
add_wg14_entrypoint(stdc_raise)
add_wg14_entrypoint(siginstall)
add_wg14_entrypoint(siguninstall)
add_wg14_entrypoint(siguninstall_system)
add_wg14_entrypoint(signal_decider_create)
add_wg14_entrypoint(signal_decider_destroy)
add_wg14_entrypoint(tss_async_signal_safe_create)
add_wg14_entrypoint(tss_async_signal_safe_destroy)
add_wg14_entrypoint(tss_async_signal_safe_thread_init)
add_wg14_entrypoint(tss_async_signal_safe_get)
add_wg14_entrypoint(current_thread_id)
