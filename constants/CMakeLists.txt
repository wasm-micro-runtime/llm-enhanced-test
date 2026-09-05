# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(constants MODES classic-interp llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# WAMR build flags for enhanced constant opcode testing (i32.const, i64.const, f32.const, f64.const)
set(WAMR_BUILD_LIBC_WASI 1)

# Include shared unit test configuration
include(../../unit_common.cmake)

# Include AOT compilation support
include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

# Set up include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Prepare unit test sources for i32.const test
set(i32_const_test_sources
        enhanced_i32_const_test.cc
        ${WAMR_RUNTIME_LIB_SOURCE}
        ${UNCOMMON_SHARED_SOURCE}
        )

# Prepare unit test sources for f32.const test
set(f32_const_test_sources
        enhanced_f32_const_test.cc
        ${WAMR_RUNTIME_LIB_SOURCE}
        ${UNCOMMON_SHARED_SOURCE}
        )

# Prepare unit test sources for f64.const test
set(f64_const_test_sources
        enhanced_f64_const_test.cc
        ${WAMR_RUNTIME_LIB_SOURCE}
        ${UNCOMMON_SHARED_SOURCE}
        )

# Create test executables
add_executable(enhanced_i32_const_test ${i32_const_test_sources})
add_executable(enhanced_f32_const_test ${f32_const_test_sources})
add_executable(enhanced_f64_const_test ${f64_const_test_sources})

# Link required libraries
target_link_libraries(enhanced_i32_const_test ${LLVM_AVAILABLE_LIBS} gtest_main)
target_link_libraries(enhanced_f32_const_test ${LLVM_AVAILABLE_LIBS} gtest_main)
target_link_libraries(enhanced_f64_const_test ${LLVM_AVAILABLE_LIBS} gtest_main)

# Post-build: Copy WASM test files to build directory for all tests
wamr_unit_test_copy_wasm_files(enhanced_i32_const_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR $<TARGET_FILE_DIR:enhanced_i32_const_test>/wasm-apps
    COMMENT "Copying WASM test files to i32_const test build directory"
)

wamr_unit_test_copy_wasm_files(enhanced_f32_const_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR $<TARGET_FILE_DIR:enhanced_f32_const_test>/wasm-apps
    COMMENT "Copying WASM test files to f32_const test build directory"
)

wamr_unit_test_copy_wasm_files(enhanced_f64_const_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR $<TARGET_FILE_DIR:enhanced_f64_const_test>/wasm-apps
    COMMENT "Copying WASM test files to f64_const test build directory"
)

# Register tests with CTest using gtest_discover_tests to support individual test case filtering
include(GoogleTest)

gtest_discover_tests(enhanced_i32_const_test
                     PROPERTIES RUN_SERIAL TRUE
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

gtest_discover_tests(enhanced_f32_const_test
                     PROPERTIES RUN_SERIAL TRUE
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

gtest_discover_tests(enhanced_f64_const_test
                     PROPERTIES RUN_SERIAL TRUE
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_const_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_const_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_const_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_const_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    foreach(_fixture_target
            enhanced_i32_const_test
            enhanced_f32_const_test
            enhanced_f64_const_test)
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
