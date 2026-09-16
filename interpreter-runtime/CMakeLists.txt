# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(interpreter-runtime MODES classic-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# No WAMR_BUILD_LIBC_WASI: these cases only use the runtime API (verified by
# building and running the suite without the switch).
set(WAMR_BUILD_THREAD_MGR 1)
set(WAMR_BUILD_MULTI_MODULE 1)
set(WAMR_BUILD_DUMP_CALL_STACK 1)

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
  ${source_all}
  ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable(interpreter-runtime ${unit_test_sources})
target_link_libraries(interpreter-runtime ${LLVM_AVAILABLE_LIBS} gtest_main)
gtest_discover_tests(interpreter-runtime)
