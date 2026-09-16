# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(wasm-c-api MODES llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# No feature switch is needed: the cases only use the C API, and the
# engine-store cases that capture the runtime output link no libc either (the
# probe builds and passes all 103 cases without libc-builtin/dump-call-stack).

include(../../unit_common.cmake)

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
  ${source_all}
  ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable(llm-enhanced-test-wasm-c-api ${unit_test_sources})
target_link_libraries(llm-enhanced-test-wasm-c-api ${LLVM_AVAILABLE_LIBS} gtest_main)
gtest_discover_tests(llm-enhanced-test-wasm-c-api)
