# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test-enhanced-simd-opcodes)

wamr_unit_test_suite_run_modes(simd MODES llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_MEMORY_PROFILING 0)

set(WAMR_BUILD_SIMD 1)

# Platform-specific definitions
add_definitions(-DRUN_ON_LINUX)

# Include shared unit test configuration
include(../../unit_common.cmake)

# Configure LLVM for AOT compilation support
set(LLVM_SRC_ROOT "${WAMR_ROOT_DIR}/core/deps/llvm")

if (NOT EXISTS "${LLVM_SRC_ROOT}/build")
    message(FATAL_ERROR "Cannot find LLVM dir: ${LLVM_SRC_ROOT}/build")
endif ()

set(CMAKE_PREFIX_PATH "${LLVM_SRC_ROOT}/build;${CMAKE_PREFIX_PATH}")
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include AOT compilation support
include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

# Set up include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Collect all test source files
file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
# Remove files from build directory to avoid conflicts
list(FILTER source_all EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}/.*")

# # Ensure f32x4.extract_lane test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f32x4_extract_lane_test.cc)

# # Ensure f64x2.extract_lane test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f64x2_extract_lane_test.cc)

# # Ensure i64x2.shl test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i64x2_shl_test.cc)

# # Ensure i32x4.extadd_pairwise_i16x8_u test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i32x4_extadd_pairwise_i16x8_u_test.cc)

# # Ensure i16x8.q15mulr_sat_s test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i16x8_q15mulr_sat_s_test.cc)

# # Ensure f64x2.convert_low_i32x4_s test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f64x2_convert_low_i32x4_s_test.cc)

# # Ensure f32x4.neg test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f32x4_neg_test.cc)
# # Ensure i8x16.avgr_u test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i8x16_avgr_u_test.cc)

# # Ensure i32x4.lt_s test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i32x4_lt_s_test.cc)

# # Ensure i16x8.narrow_i32x4_u test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i16x8_narrow_i32x4_u_test.cc)

# # Ensure i16x8.extend_low_i8x16_s test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i16x8_extend_low_i8x16_s_test.cc)

# # Ensure f64x2.promote_low_f32x4 test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f64x2_promote_low_f32x4_test.cc)

# # Ensure i64x2.gt_u test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i64x2_gt_u_test.cc)

# # Ensure i8x16.narrow_i16x8_s test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i8x16_narrow_i16x8_s_test.cc)

# # Ensure i64x2.extend_high_i32x4 test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i64x2_extend_high_i32x4_test.cc)

# # Ensure i32x4.extend_low_i16x8 test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i32x4_extend_low_i16x8_test.cc)

# # Ensure i16x8.extadd_pairwise_i8x16 test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i16x8_extadd_pairwise_i8x16_test.cc)

# # Ensure i64x2.extmul_low_i32x4 test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i64x2_extmul_low_i32x4_test.cc)

# # Ensure f32x4.abs test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f32x4_abs_test.cc)

# # Ensure f64x2.neg test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f64x2_neg_test.cc)

# # Ensure f32x4.div test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f32x4_div_test.cc)

# # Ensure f64x2.sub test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_f64x2_sub_test.cc)

# # Ensure i64x2.sub test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i64x2_sub_test.cc)

# # Ensure i8x16.sub test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i8x16_sub_test.cc)

# # Ensure i32x4.lt test is included
# list(APPEND source_all ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_i32x4_lt_test.cc)

set(UNIT_SOURCE ${source_all})

# Prepare unit test sources
set(unit_test_sources
        ${UNIT_SOURCE}
        ${WAMR_RUNTIME_LIB_SOURCE}
        ${UNCOMMON_SHARED_SOURCE}
        )

# Create test executable
add_executable(enhanced_simd_test ${unit_test_sources})

# Link required libraries
target_link_libraries(enhanced_simd_test ${LLVM_AVAILABLE_LIBS} gtest_main)
# Enable test discovery
include(GoogleTest)
gtest_discover_tests(enhanced_simd_test)

# Copy WASM test files to build directory for test execution
wamr_unit_test_copy_wasm_files(enhanced_simd_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMENT "Copying WASM test files to build directory"
)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_abs_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_abs_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_ceil_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_ceil_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_div_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_div_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_eq_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_eq_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_extract_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_extract_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_floor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_floor_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_max_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_max_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_min_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_min_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_ne_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_ne_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_nearest_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_nearest_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_neg_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_neg_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_pmax_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_pmax_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_pmin_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_pmin_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_replace_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_replace_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32x4_trunc_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32x4_trunc_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_abs_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_abs_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_ceil_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_ceil_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_convert_low_i32x4_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_convert_low_i32x4_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_demote_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_demote_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_eq_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_eq_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_extmul_low_i32x4_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_extmul_low_i32x4_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_extract_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_extract_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_floor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_floor_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_max_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_max_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_min_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_min_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_ne_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_ne_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_nearest_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_nearest_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_neg_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_neg_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_pmax_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_pmax_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_pmin_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_pmin_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_promote_low_f32x4_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_promote_low_f32x4_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_sqrt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_sqrt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_trunc_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_trunc_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i16x8_abs_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i16x8_abs_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i16x8_extadd_pairwise_i8x16_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i16x8_extadd_pairwise_i8x16_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i16x8_extmul_high_i8x16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i16x8_extmul_high_i8x16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i16x8_extmul_low_i8x16_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i16x8_extmul_low_i8x16_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i16x8_narrow_i32x4_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i16x8_narrow_i32x4_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i16x8_q15mulr_sat_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i16x8_q15mulr_sat_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_abs_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_abs_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_dot_i16x8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_dot_i16x8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_extadd_pairwise_i16x8_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_extadd_pairwise_i16x8_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_extend_low_i16x8_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_extend_low_i16x8_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_extmul_high_i16x8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_extmul_high_i16x8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_extmul_high_i16x8_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_extmul_high_i16x8_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_extmul_low_i16x8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_extmul_low_i16x8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_extmul_low_i16x8_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_extmul_low_i16x8_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_lt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_lt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_lt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_lt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_trunc_sat_f64x2_s_zero_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_trunc_sat_f64x2_s_zero_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_trunc_sat_f64x2_u_zero_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_trunc_sat_f64x2_u_zero_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64x2_extend_high_i32x4_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64x2_extend_high_i32x4_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64x2_extmul_low_i32x4_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64x2_extmul_low_i32x4_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64x2_gt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64x2_gt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64x2_shl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64x2_shl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64x2_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64x2_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_abs_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_abs_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_all_true_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_all_true_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_avgr_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_avgr_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_bitmask_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_bitmask_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_eq_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_eq_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_extract_lane_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_extract_lane_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_narrow_i16x8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_narrow_i16x8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_ne_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_ne_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_popcnt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_popcnt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_shl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_shl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_shuffle_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_shuffle_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_swizzle_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_swizzle_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_and_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_and_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_andnot_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_andnot_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_any_true_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_any_true_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_bitselect_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_bitselect_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load16_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load16_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load32_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load32_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load32_zero_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load32_zero_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load64_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load64_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load64_zero_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load64_zero_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load8_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load8_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load8x8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load8x8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load8x8_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load8x8_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_not_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_not_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_or_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_or_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_store16_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_store16_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_store32_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_store32_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_store64_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_store64_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_store8_lane_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_store8_lane_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_xor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_xor_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET enhanced_simd_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
