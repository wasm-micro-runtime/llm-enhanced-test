# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project (interpreter-invoke)

wamr_unit_test_suite_run_modes(interpreter-invoke MODES classic-interp fast-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions (-DRUN_ON_LINUX)

add_definitions (-Dattr_container_malloc=malloc)
add_definitions (-Dattr_container_free=free)
# add_definitions (-DWASM_ENABLE_WAMR_COMPILER=1)

set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_APP_FRAMEWORK 1)

include (../../unit_common.cmake)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

file (GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set (UNIT_SOURCE ${source_all})

set (unit_test_sources
  ${UNIT_SOURCE}
  ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable (interpreter-invoke-classic ${unit_test_sources})
target_compile_definitions(interpreter-invoke-classic PUBLIC -DWAMR_BUILD_FAST_INTERP=0)
target_link_libraries (interpreter-invoke-classic gtest_main)

add_executable (interpreter-invoke-fast ${unit_test_sources})
target_compile_definitions(interpreter-invoke-fast PUBLIC -DWAMR_BUILD_FAST_INTERP=1)
target_link_libraries (interpreter-invoke-fast gtest_main)

gtest_discover_tests(interpreter-invoke-classic)
gtest_discover_tests(interpreter-invoke-fast)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/function_invocation_test.wat|${CMAKE_CURRENT_BINARY_DIR}/function_invocation_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    foreach(_fixture_target
            interpreter-invoke-classic
            interpreter-invoke-fast)
        wamr_unit_test_compile_wat_to_wasm(
            TARGET ${_fixture_target}
            SOURCE ${_fixture_source}
            OUTPUT ${_fixture_output}
        )
    endforeach()
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_target)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
