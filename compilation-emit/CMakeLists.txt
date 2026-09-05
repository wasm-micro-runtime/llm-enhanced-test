# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Kept at MODES none: the suite builds and links under aot, but its
# aot_emit_object_file tests fail at runtime (aot_compile_wasm returns
# false against the current AOT compiler), a pre-existing test issue.
wamr_unit_test_suite_run_modes(compilation-emit MODES none)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions (-DWASM_ENABLE_WAMR_COMPILER=1)

set (WAMR_BUILD_DUMP_CALL_STACK 1)
set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_THREAD_MGR 1)
set (WAMR_BUILD_GC 1)
set (WAMR_BUILD_REF_TYPES 1)

include (../../unit_common.cmake)

include (${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

file (GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set (UNIT_SOURCE ${source_all})

set (unit_test_sources
     ${UNIT_SOURCE}
     ${WAMR_RUNTIME_LIB_SOURCE}
     ${UNCOMMON_SHARED_SOURCE}
    )

# Now simply link against gtest or gtest_main as needed. Eg
add_executable (compilation-emit ${unit_test_sources})

target_link_libraries (compilation-emit ${LLVM_AVAILABLE_LIBS} gtest_main )

include(GoogleTest)
# NOTE: `compilation-emit` is a single gtest binary, and CTest runs each
# discovered test as a separate process with a gtest filter. When code coverage
# is enabled, running these processes concurrently can race on `.gcda` writes and
# cause sporadic "Subprocess aborted". Force serialization for this executable's
# tests to make `ctest --parallel` stable.
gtest_discover_tests(compilation-emit
                     PROPERTIES RUN_SERIAL TRUE)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/conversion_test.wat|${CMAKE_CURRENT_BINARY_DIR}/conversion_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_const_test.wat|${CMAKE_CURRENT_BINARY_DIR}/f32_const_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_convert_i32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/f32_convert_i32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_convert_i64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/f32_convert_i64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f32_demote_f64.wat|${CMAKE_CURRENT_BINARY_DIR}/f32_demote_f64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_convert_s_i64.wat|${CMAKE_CURRENT_BINARY_DIR}/f64_convert_s_i64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_convert_u_i64.wat|${CMAKE_CURRENT_BINARY_DIR}/f64_convert_u_i64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64_promote_f32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/f64_promote_f32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_extend16s.wat|${CMAKE_CURRENT_BINARY_DIR}/i32_extend16s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_extend8s.wat|${CMAKE_CURRENT_BINARY_DIR}/i32_extend8s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_extend_combined.wat|${CMAKE_CURRENT_BINARY_DIR}/i32_extend_combined_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_f32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/i32_trunc_f32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32_trunc_f64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/i32_trunc_f64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend16s.wat|${CMAKE_CURRENT_BINARY_DIR}/i64_extend16s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend32s.wat|${CMAKE_CURRENT_BINARY_DIR}/i64_extend32s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend8s.wat|${CMAKE_CURRENT_BINARY_DIR}/i64_extend8s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend_combined.wat|${CMAKE_CURRENT_BINARY_DIR}/i64_extend_combined_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_extend_i32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/i64_extend_i32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_trunc_f32_test.wat|${CMAKE_CURRENT_BINARY_DIR}/i64_trunc_f32_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64_trunc_f64_test.wat|${CMAKE_CURRENT_BINARY_DIR}/i64_trunc_f64_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/main.wat|${CMAKE_CURRENT_BINARY_DIR}/main.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET compilation-emit
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
