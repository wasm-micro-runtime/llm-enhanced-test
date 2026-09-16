# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(wasm-c-api MODES llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# The engine-store cases redirect the runtime output and dump the call stack.
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_DUMP_CALL_STACK 1)

include(../../unit_common.cmake)

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
  ${source_all}
  ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable(llm-enhanced-test-wasm-c-api ${unit_test_sources})
target_link_libraries(llm-enhanced-test-wasm-c-api ${LLVM_AVAILABLE_LIBS} gtest_main)
gtest_discover_tests(llm-enhanced-test-wasm-c-api)
