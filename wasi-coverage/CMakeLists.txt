# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(wasi-coverage-test)

wamr_unit_test_suite_run_modes(wasi-coverage MODES classic-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_BULK_MEMORY 1)
set(WAMR_BUILD_REF_TYPES 1)

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
list(FILTER source_all EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}/.*")

set(unit_test_sources
    ${source_all}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${UNCOMMON_SHARED_SOURCE}
)

add_executable(wasi_coverage_test ${unit_test_sources})
target_link_libraries(wasi_coverage_test ${LLVM_AVAILABLE_LIBS} gtest_main)

gtest_discover_tests(wasi_coverage_test)

wamr_unit_test_copy_wasm_files(wasi_coverage_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMENT "Copying WASI test WASM files"
)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/wasi_env_args.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/wasi_env_args.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/wasi_fd_ops.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/wasi_fd_ops.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/wasi_path_ops.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/wasi_path_ops.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/wasi_poll.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/wasi_poll.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET wasi_coverage_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
