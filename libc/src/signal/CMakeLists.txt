if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${LIBC_TARGET_OS})
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${LIBC_TARGET_OS})
endif()

# The wg14_signals reference implementation (WG14 N3924 improved C signals)
# submodule. Built on the platforms whose OS facilities it needs (POSIX
# sigaction/pthread_kill and friends). Currently only Linux: the embedder
# shim (wg14/embedder_shim.c) uses Linux syscalls; darwin/freebsd bring-up
# (Phase 5/6 of plans/llvm-project-fork.md) adds their syscall layers and
# widens this gate.
if(LIBC_TARGET_OS STREQUAL "linux")
  add_subdirectory(wg14-build)
endif()

add_entrypoint_object(
  raise
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.raise
)

add_entrypoint_object(
  kill
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.kill
)

add_entrypoint_object(
  sigaction
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.sigaction
)

add_entrypoint_object(
  sigaltstack
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.sigaltstack
)

add_entrypoint_object(
  sigprocmask
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.sigprocmask
)

add_entrypoint_object(
  pthread_sigmask
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.pthread_sigmask
)

add_entrypoint_object(
  sigemptyset
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.sigemptyset
)

add_entrypoint_object(
  sigaddset
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.sigaddset
)

add_entrypoint_object(
  signal
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.signal
)

add_entrypoint_object(
  sigfillset
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.sigfillset
)

add_entrypoint_object(
  sigismember
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.sigismember
)


# WG14 N3924 improved C signals entrypoints (wg14_signals submodule).
# The platform gate for the submodule itself lives in the wg14/CMakeLists.txt
# add_subdirectory call above; on platforms without it the DEPENDS below has
# no target and the config files simply do not list these entrypoints.
set(wg14_entrypoints
  sigfillset_synchronous
  sigfillset_asynchronous_nondebug
  sigfillset_asynchronous_debug
  sigguarded
  sigdecider_abandon
  sigdecider_abandon_resume
  stdc_raise
  siginstall
  siguninstall
  siguninstall_system
  signal_decider_create
  signal_decider_destroy
)
# The <threads.h> domain (tss_async_signal_safe_*, current_thread_id) is
# registered in libc/src/threads/CMakeLists.txt.
foreach(entry IN LISTS wg14_entrypoints)
  add_entrypoint_object(
    ${entry}
    ALIAS
    DEPENDS
      .wg14-build.entrypoints.${entry}
  )
endforeach()

add_entrypoint_object(
  sigdelset
  ALIAS
  DEPENDS
    .${LIBC_TARGET_OS}.sigdelset
)
