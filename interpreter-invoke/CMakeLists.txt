# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(interpreter-invoke
  MODES classic-interp fast-interp
)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
  ${source_all}
  ${WAMR_RUNTIME_LIB_SOURCE}
)

# One test binary per supported running mode; the mode is selected by the
# configure command line, so only the matching binary is created (and
# registered with ctest).
if(WAMR_BUILD_FAST_INTERP EQUAL 1)
  set(_interpreter_invoke_target interpreter-invoke-fast)
else()
  set(_interpreter_invoke_target interpreter-invoke-classic)
endif()

add_executable(${_interpreter_invoke_target} ${unit_test_sources})
target_link_libraries(${_interpreter_invoke_target} gtest_main)
gtest_discover_tests(${_interpreter_invoke_target})

# Generated WASM fixture declarations

wamr_unit_test_compile_wat_to_wasm(
    TARGET ${_interpreter_invoke_target}
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/function_invocation_test.wat
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/function_invocation_test.wasm
)

unset(_interpreter_invoke_target)
