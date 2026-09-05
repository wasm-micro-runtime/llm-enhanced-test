# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(memory MODES llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# Enable required WAMR build flags for memory opcode testing (memory.init, i32.load, i32.store, i32.store8, i32.store16, f64.store)
set(WAMR_BUILD_LIBC_WASI 1)

# Enable standard memory operations for i32.load, i32.store, i32.store8, i32.store16, and f64.store opcodes (enabled by default)
# Memory load/store operations are part of the core WebAssembly specification

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
add_executable(enhanced_memory_opcodes_test ${unit_test_sources})

# Link required libraries
target_link_libraries(enhanced_memory_opcodes_test ${LLVM_AVAILABLE_LIBS} gtest_main)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(enhanced_memory_opcodes_test)

# Copy WASM test files to build directory for test execution
wamr_unit_test_copy_wasm_files(enhanced_memory_opcodes_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMENT "Copying WASM test files to build directory"
)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_load_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_store_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_store_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_load_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_store_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_store_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_load16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_load16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_load16_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_load16_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_load8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_load8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_load8_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_load8_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_load_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_store16_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_store16_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_store8_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_store8_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_store_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_store_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_load16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_load16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_load16_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_load16_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_load32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_load32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_load32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_load32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_load8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_load8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_load8_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_load8_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_load_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_store16_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_store16_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_store32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_store32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_store8_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_store8_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_store_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_store_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_copy_error_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_copy_error_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_copy_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_copy_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_fill_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_fill_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_grow_limited_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_grow_limited_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_grow_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_grow_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_init_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_init_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_size_0page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_size_0page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_size_1000page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_size_1000page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_size_16page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_size_16page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_size_1page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_size_1page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_size_4page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_size_4page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/memory_size_limited_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory_size_limited_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET enhanced_memory_opcodes_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
