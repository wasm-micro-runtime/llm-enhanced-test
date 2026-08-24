# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(fast-interp-coverage)

wamr_unit_test_suite_run_modes(fast-interp MODES fast-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_MEMORY_PROFILING 0)
set(WAMR_BUILD_BULK_MEMORY 1)
set(WAMR_BUILD_REF_TYPES 1)
set(WAMR_BUILD_TAIL_CALL 1)

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
list(FILTER source_all EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}/.*")

set(unit_test_sources
    ${source_all}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${UNCOMMON_SHARED_SOURCE}
)

add_executable(fast_interp_coverage_test ${unit_test_sources})
target_link_libraries(fast_interp_coverage_test ${LLVM_AVAILABLE_LIBS} gtest_main)

gtest_discover_tests(fast_interp_coverage_test)

# Copy wasm-apps from all opcode test directories
set(WASM_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../table/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../constants/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps
)

foreach(WASM_DIR ${WASM_DIRS})
    if(EXISTS ${WASM_DIR})
        get_filename_component(PARENT_DIR ${WASM_DIR} DIRECTORY)
        get_filename_component(CATEGORY ${PARENT_DIR} NAME)
        wamr_unit_test_copy_wasm_files(fast_interp_coverage_test
            SOURCE_DIR ${WASM_DIR}
            DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${CATEGORY}
            COMMENT "Copying ${CATEGORY} WASM files"
        )
    endif()
endforeach()

# Generated WASM fixture declarations

# Generate numeric WASM fixtures from WAT/WAST sources.
set(_generated_numeric_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_abs_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_abs_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_ceil_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_ceil_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_copysign_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_copysign_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_div_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_div_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_floor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_floor_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_gt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_gt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_lt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_lt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_max_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_max_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_min_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_min_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_mul_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_mul_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_nearest_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_nearest_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_reinterpret_i32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_reinterpret_i32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_sqrt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_sqrt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f32_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f32_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_convert_i32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_convert_i32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_convert_i32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_convert_i32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_convert_i64_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_convert_i64_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_convert_i64_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_convert_i64_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_copysign_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_copysign_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_div_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_div_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_floor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_floor_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_ge_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_ge_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_gt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_gt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_le_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_le_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_max_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_max_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_min_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_min_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_mul_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_mul_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_mul_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_mul_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_nearest_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_nearest_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_promote_f32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_promote_f32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_promote_f32_underflow_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_promote_f32_underflow_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_reinterpret_i64_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_reinterpret_i64_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_reinterpret_i64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_reinterpret_i64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_sqrt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_sqrt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_sub_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_sub_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/f64_trunc_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/f64_trunc_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_add_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_add_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_add_underflow_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_add_underflow_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_and_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_and_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_clz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_clz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_ctz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_ctz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_div_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_div_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_div_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_div_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_eq_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_eq_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_eq_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_eq_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_eqz_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_eqz_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_eqz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_eqz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_extend16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_extend16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_extend8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_extend8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_gt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_gt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_gt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_gt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_le_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_le_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_le_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_le_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_lt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_lt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_lt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_lt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_mul_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_mul_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_mul_underflow_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_mul_underflow_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_ne_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_ne_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_ne_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_ne_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_popcnt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_popcnt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_reinterpret_f32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_reinterpret_f32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_rem_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_rem_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_rem_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_rem_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_rotl_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_rotl_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_rotl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_rotl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_rotr_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_rotr_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_rotr_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_rotr_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_shl_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_shl_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_shl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_shl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_shr_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_shr_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_shr_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_shr_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_sub_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_sub_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_trunc_f32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_trunc_f32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_trunc_f32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_trunc_f32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_trunc_f64_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_trunc_f64_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_trunc_sat_f32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_trunc_sat_f32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_trunc_sat_f32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_trunc_sat_f32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_trunc_sat_f64_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_trunc_sat_f64_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_trunc_sat_f64_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_trunc_sat_f64_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_wrap_i64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_wrap_i64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_xor_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_xor_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i32_xor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i32_xor_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_clz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_clz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_ctz_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_ctz_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_extend16_s_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_extend16_s_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_extend16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_extend16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_extend32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_extend32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_extend8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_extend8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_extend_i32_s_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_extend_i32_s_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_extend_i32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_extend_i32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_extend_i32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_extend_i32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_ge_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_ge_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_ge_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_ge_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_gt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_gt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_gt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_gt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_le_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_le_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_le_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_le_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_lt_s_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_lt_s_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_lt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_lt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_lt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_lt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_popcnt_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_popcnt_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_popcnt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_popcnt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_reinterpret_f64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_reinterpret_f64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_rem_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_rem_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_rem_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_rem_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_rotl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_rotl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_rotr_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_rotr_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_shl_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_shl_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_shl_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_shl_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_shr_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_shr_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_shr_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_shr_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_trunc_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_trunc_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_trunc_sat_f32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_trunc_sat_f32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_trunc_sat_f32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_trunc_sat_f32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_trunc_sat_f64_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_trunc_sat_f64_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_trunc_sat_f64_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_trunc_sat_f64_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_trunc_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_trunc_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_xor_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_xor_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps/i64_xor_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/numeric/i64_xor_test.wasm
)
foreach(_fixture IN LISTS _generated_numeric_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET fast_interp_coverage_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_numeric_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)


# Generate memory WASM fixtures from WAT/WAST sources.
set(_generated_memory_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/f32_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/f32_load_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/f32_store_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/f32_store_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/f64_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/f64_load_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/f64_store_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/f64_store_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i32_load16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i32_load16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i32_load16_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i32_load16_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i32_load8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i32_load8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i32_load8_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i32_load8_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i32_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i32_load_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i32_store16_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i32_store16_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i32_store8_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i32_store8_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i32_store_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i32_store_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_load16_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_load16_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_load16_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_load16_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_load32_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_load32_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_load32_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_load32_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_load8_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_load8_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_load8_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_load8_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_load_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_store16_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_store16_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_store32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_store32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_store8_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_store8_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/i64_store_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/i64_store_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_copy_error_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_copy_error_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_copy_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_copy_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_fill_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_fill_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_grow_limited_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_grow_limited_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_grow_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_grow_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_init_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_init_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_size_0page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_size_0page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_size_1000page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_size_1000page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_size_16page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_size_16page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_size_1page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_size_1page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_size_4page_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_size_4page_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps/memory_size_limited_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/memory/memory_size_limited_test.wasm
)
foreach(_fixture IN LISTS _generated_memory_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET fast_interp_coverage_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_memory_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)


# Generate control-flow WASM fixtures from WAT/WAST sources.
set(_generated_control_flow_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/block_bt_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/block_bt_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/call_indirect_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/call_indirect_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/call_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/call_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/end_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/end_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/return_call_error_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/return_call_error_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/return_call_indirect_error_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/return_call_indirect_error_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/return_call_indirect_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/return_call_indirect_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/return_call_ref_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/return_call_ref_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/return_call_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/return_call_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps/select_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/control-flow/select_test.wasm
)
foreach(_fixture IN LISTS _generated_control_flow_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET fast_interp_coverage_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_control_flow_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)


# Generate variable WASM fixtures from WAT/WAST sources.
set(_generated_variable_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/drop_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/drop_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/drop_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/drop_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/global_get_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/global_get_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/global_set_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/global_set_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/local_get_invalid_index_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/local_get_invalid_index_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/local_get_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/local_get_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/local_set_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/local_set_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/local_tee_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/local_tee_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps/local_tee_type_mismatch_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/variable/local_tee_type_mismatch_test.wasm
)
foreach(_fixture IN LISTS _generated_variable_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET fast_interp_coverage_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_variable_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)


# Generate table WASM fixtures from WAT/WAST sources.
set(_generated_table_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/../table/wasm-apps/elem_drop_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/table/elem_drop_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../table/wasm-apps/table_drop_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/table/table_drop_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../table/wasm-apps/table_init_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/table/table_init_test.wasm
)
foreach(_fixture IN LISTS _generated_table_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET fast_interp_coverage_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_table_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)


# Generate constants WASM fixtures from WAT/WAST sources.
set(_generated_constants_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/../constants/wasm-apps/f32_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/constants/f32_const_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../constants/wasm-apps/f64_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/constants/f64_const_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../constants/wasm-apps/i32_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/constants/i32_const_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../constants/wasm-apps/i64_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/constants/i64_const_test.wasm
)
foreach(_fixture IN LISTS _generated_constants_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET fast_interp_coverage_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_constants_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)


# Generate reference WASM fixtures from WAT/WAST sources.
set(_generated_reference_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/any_convert_extern_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/any_convert_extern_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/any_convert_extern_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/any_convert_extern_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/call_ref_null_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/call_ref_null_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/call_ref_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/call_ref_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/extern_convert_any_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/extern_convert_any_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/ref_as_non_null_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/ref_as_non_null_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/ref_as_non_null_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/ref_as_non_null_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/ref_eq_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/ref_eq_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/ref_eq_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/ref_eq_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/table_get_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/table_get_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/table_size_invalid_index.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/table_size_invalid_index.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps/table_size_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/reference/table_size_test.wasm
)
foreach(_fixture IN LISTS _generated_reference_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET fast_interp_coverage_test
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_reference_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
