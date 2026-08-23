# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(interpreter-core)

wamr_unit_test_suite_run_modes(interpreter-core MODES classic-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions(-DRUN_ON_LINUX)
add_definitions(-Dattr_container_malloc=malloc)
add_definitions(-Dattr_container_free=free)

set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_APP_FRAMEWORK 1)

if(WAMR_BUILD_TARGET MATCHES "X86_.*")
    add_definitions(-DBUILD_TARGET_X86=1)
elseif(WAMR_BUILD_TARGET MATCHES "AARCH64.*")
    add_definitions(-DBUILD_TARGET_AARCH64=1)
elseif(WAMR_BUILD_TARGET MATCHES "ARM.*")
    add_definitions(-DBUILD_TARGET_ARM=1)
elseif(WAMR_BUILD_TARGET MATCHES "RISCV.*")
    add_definitions(-DBUILD_TARGET_RISCV=1)
endif()

if(WAMR_BUILD_SIMD EQUAL 1)
    add_definitions(-DWASM_ENABLE_SIMD=1)
else()
    add_definitions(-DWASM_ENABLE_SIMD=0)
endif()

if(WAMR_BUILD_JIT EQUAL 1)
    add_definitions(-DWASM_ENABLE_JIT=1)
else()
    add_definitions(-DWASM_ENABLE_JIT=0)
endif()

if(WAMR_BUILD_FAST_JIT EQUAL 1)
    add_definitions(-DWASM_ENABLE_FAST_JIT=1)
else()
    add_definitions(-DWASM_ENABLE_FAST_JIT=0)
endif()

if(WAMR_BUILD_MEMORY64 EQUAL 1)
    add_definitions(-DWASM_ENABLE_MEMORY64=1)
else()
    add_definitions(-DWASM_ENABLE_MEMORY64=0)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
    ${source_all}
    ${PLATFORM_SHARED_SOURCE}
    ${UTILS_SHARED_SOURCE}
    ${MEM_ALLOC_SHARED_SOURCE}
    ${NATIVE_INTERFACE_SOURCE}
    ${LIBC_BUILTIN_SOURCE}
    ${IWASM_COMMON_SOURCE}
    ${IWASM_INTERP_SOURCE}
    ${WAMR_ROOT_DIR}/core/shared/utils/uncommon/bh_read_file.c
)

add_executable(interpreter-core ${unit_test_sources})

target_include_directories(interpreter-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${WAMR_ROOT_DIR}/core/iwasm/interpreter
    ${WAMR_ROOT_DIR}/core/iwasm/include
    ${WAMR_ROOT_DIR}/core/iwasm/common
    ${WAMR_ROOT_DIR}/core/shared/utils
    ${WAMR_ROOT_DIR}/core/shared/utils/uncommon
    ${WAMR_ROOT_DIR}/core/shared/platform/include
)

target_link_libraries(interpreter-core
    gtest_main
    ${LLVM_AVAILABLE_LIBS}
)

target_compile_definitions(interpreter-core PRIVATE
    WASM_ENABLE_INTERP=1
    WASM_ENABLE_FAST_INTERP=0
    WASM_ENABLE_LIBC_BUILTIN=1
    WASM_ENABLE_MULTI_MODULE=1
    WASM_ENABLE_TAIL_CALL=1
    WASM_ENABLE_BULK_MEMORY=1
    WASM_ENABLE_REF_TYPES=1
)

include(GoogleTest)
gtest_discover_tests(interpreter-core)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/arithmetic_bitwise.wat|${CMAKE_CURRENT_BINARY_DIR}/arithmetic_bitwise.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/complex_sections_module.wat|${CMAKE_CURRENT_BINARY_DIR}/complex_sections_module.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/datacount_test.wat|${CMAKE_CURRENT_BINARY_DIR}/datacount_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/empty_table_segments.wat|${CMAKE_CURRENT_BINARY_DIR}/empty_table_segments.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/floating_point_operations.wat|${CMAKE_CURRENT_BINARY_DIR}/floating_point_operations.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/function_invocation_test.wat|${CMAKE_CURRENT_BINARY_DIR}/function_invocation_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/import_export_module.wat|${CMAKE_CURRENT_BINARY_DIR}/import_export_module.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/invalid_magic_number.wat|${CMAKE_CURRENT_BINARY_DIR}/invalid_magic_number.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/multiple_table_types.wat|${CMAKE_CURRENT_BINARY_DIR}/multiple_table_types.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/simd_shuffle_test.wat|${CMAKE_CURRENT_BINARY_DIR}/simd_shuffle_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/table_validation_test.wat|${CMAKE_CURRENT_BINARY_DIR}/table_validation_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/valid_basic_module.wat|${CMAKE_CURRENT_BINARY_DIR}/valid_basic_module.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET interpreter-core
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
