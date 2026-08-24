# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test-enhanced-table-opcodes)

wamr_unit_test_suite_run_modes(table MODES classic-interp llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# Enable required WAMR build flags for table opcode testing
set(WAMR_BUILD_BULK_MEMORY 1)       # Required for table.init opcode
set(WAMR_BUILD_REF_TYPES 1)         # Required for table operations with references
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_MEMORY_PROFILING 0)

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
add_executable(enhanced_table_opcodes_test ${unit_test_sources})

# Link required libraries
target_link_libraries(enhanced_table_opcodes_test ${LLVM_AVAILABLE_LIBS} gtest_main)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(enhanced_table_opcodes_test)

# Copy WASM test files to build directory for test execution
wamr_unit_test_copy_wasm_files(enhanced_table_opcodes_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMENT "Copying WASM test files to build directory"
)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/elem_drop_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/elem_drop_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/table_drop_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/table_drop_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/table_init_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/table_init_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET enhanced_table_opcodes_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
