# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test-enhanced-reference-opcodes)

wamr_unit_test_suite_run_modes(reference MODES classic-interp llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# Enable required WAMR build flags for reference type opcodes (any.convert_extern, extern.convert_any, table operations)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_MEMORY_PROFILING 0)

# Enable bulk memory operations and reference types for reference opcodes
set(WAMR_BUILD_BULK_MEMORY 1)
set(WAMR_BUILD_REF_TYPES 1)

# Enable function references for call_ref opcode support
set(WAMR_BUILD_FUNCTION_REFERENCES 1)

# Enable GC features for any.convert_extern opcode support
# set(WAMR_BUILD_GC 1)

# Platform-specific definitions
add_definitions(-DRUN_ON_LINUX)

# Include shared unit test configuration
include(../../unit_common.cmake)

# Include AOT compilation support
include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

# Set up include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Collect all test source files
file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
# Remove files from build directory to avoid conflicts
list(FILTER source_all EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}/.*")

set(UNIT_SOURCE ${source_all})

# Prepare unit test sources
set(unit_test_sources
        ${UNIT_SOURCE}
        ${WAMR_RUNTIME_LIB_SOURCE}
        ${UNCOMMON_SHARED_SOURCE}
        )

# Create test executable
add_executable(enhanced_reference_test ${unit_test_sources})

# Link required libraries
target_link_libraries(enhanced_reference_test ${LLVM_AVAILABLE_LIBS} gtest_main)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(enhanced_reference_test)

# Copy WASM test files to build directory for test execution
wamr_unit_test_copy_wasm_files(enhanced_reference_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMENT "Copying WASM test files to build directory"
)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/any_convert_extern_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/any_convert_extern_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/any_convert_extern_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/any_convert_extern_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/call_ref_null_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/call_ref_null_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/call_ref_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/call_ref_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/extern_convert_any_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/extern_convert_any_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/ref_as_non_null_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/ref_as_non_null_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/ref_as_non_null_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/ref_as_non_null_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/ref_eq_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/ref_eq_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/ref_eq_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/ref_eq_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/table_get_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/table_get_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/table_size_invalid_index.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/table_size_invalid_index.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/table_size_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/table_size_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET enhanced_reference_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
