# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(numeric MODES llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

set(WAMR_BUILD_LIBC_WASI 1)

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
add_executable(enhanced_numeric_opcodes_test ${unit_test_sources})

# Link required libraries
target_link_libraries(enhanced_numeric_opcodes_test ${LLVM_AVAILABLE_LIBS} gtest_main)
# Enable test discovery
include(GoogleTest)
gtest_discover_tests(enhanced_numeric_opcodes_test)

# Copy WASM test files to build directory for test execution
wamr_unit_test_copy_wasm_files(enhanced_numeric_opcodes_test
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMENT "Copying WASM test files to build directory"
)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_abs_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_abs_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_ceil_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_ceil_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_copysign_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_copysign_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_div_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_div_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_floor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_floor_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_gt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_gt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_lt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_lt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_max_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_max_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_min_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_min_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_mul_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_mul_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_nearest_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_nearest_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_reinterpret_i32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_reinterpret_i32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_sqrt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_sqrt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f32_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_convert_i32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_convert_i32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_convert_i32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_convert_i32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_convert_i64_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_convert_i64_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_convert_i64_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_convert_i64_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_copysign_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_copysign_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_div_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_div_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_floor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_floor_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_ge_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_ge_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_gt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_gt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_le_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_le_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_max_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_max_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_min_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_min_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_mul_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_mul_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_mul_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_mul_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_nearest_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_nearest_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_promote_f32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_promote_f32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_promote_f32_underflow_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_promote_f32_underflow_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_reinterpret_i64_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_reinterpret_i64_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_reinterpret_i64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_reinterpret_i64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_sqrt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_sqrt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_sub_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_sub_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_trunc_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64_trunc_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_add_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_add_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_add_underflow_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_add_underflow_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_and_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_and_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_clz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_clz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_ctz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_ctz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_div_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_div_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_div_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_div_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_eq_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_eq_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_eq_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_eq_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_eqz_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_eqz_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_eqz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_eqz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_extend16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_extend16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_extend8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_extend8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_gt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_gt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_gt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_gt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_le_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_le_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_le_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_le_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_lt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_lt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_lt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_lt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_mul_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_mul_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_mul_underflow_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_mul_underflow_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_ne_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_ne_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_ne_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_ne_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_popcnt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_popcnt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_reinterpret_f32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_reinterpret_f32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_rem_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_rem_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_rem_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_rem_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_rotl_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_rotl_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_rotl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_rotl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_rotr_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_rotr_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_rotr_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_rotr_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_shl_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_shl_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_shl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_shl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_shr_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_shr_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_shr_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_shr_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_f32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_trunc_f32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_f32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_trunc_f32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_f64_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_trunc_f64_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_sat_f32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_trunc_sat_f32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_sat_f32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_trunc_sat_f32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_sat_f64_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_trunc_sat_f64_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_sat_f64_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_trunc_sat_f64_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_wrap_i64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_wrap_i64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_xor_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_xor_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_xor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32_xor_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_clz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_clz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_ctz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_ctz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend16_s_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_extend16_s_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_extend16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_extend32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_extend8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend_i32_s_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_extend_i32_s_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend_i32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_extend_i32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend_i32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_extend_i32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_ge_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_ge_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_ge_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_ge_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_gt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_gt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_gt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_gt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_le_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_le_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_le_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_le_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_lt_s_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_lt_s_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_lt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_lt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_lt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_lt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_popcnt_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_popcnt_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_popcnt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_popcnt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_reinterpret_f64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_reinterpret_f64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_rem_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_rem_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_rem_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_rem_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_rotl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_rotl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_rotr_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_rotr_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_shl_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_shl_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_shl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_shl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_shr_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_shr_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_shr_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_shr_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_trunc_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_trunc_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_trunc_sat_f32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_trunc_sat_f32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_trunc_sat_f32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_trunc_sat_f32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_trunc_sat_f64_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_trunc_sat_f64_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_trunc_sat_f64_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_trunc_sat_f64_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_trunc_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_trunc_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_xor_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_xor_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_xor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64_xor_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET enhanced_numeric_opcodes_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
