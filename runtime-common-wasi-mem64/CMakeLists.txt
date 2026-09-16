# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(runtime-common-wasi-mem64 MODES aot)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# THREAD_MGR and LIB_PTHREAD are needed at link time: the cases call
# wasm_runtime_spawn_exec_env() / wasm_runtime_destroy_spawned_exec_env(),
# which the thread manager provides.  The group probe confirms it: removing
# them together fails the aot build (and per-switch probes that pass have been
# wrong here before), so do not remove them without a full build.
# WAMR_BUILD_MEMORY64 is the configuration this suite exists to cover.
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_MEMORY64 1)
set(WAMR_BUILD_LIB_PTHREAD 1)
set(WAMR_BUILD_THREAD_MGR 1)

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${WAMR_ROOT_DIR}/tests/unit/common)

set(UNIT_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime_common_enhanced_test.cc
)

set(unit_test_sources
  ${UNIT_SOURCE}
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
)

add_executable(runtime-common-wasi-mem64 ${unit_test_sources})

target_link_libraries(runtime-common-wasi-mem64 gtest_main)

gtest_discover_tests(runtime-common-wasi-mem64)
