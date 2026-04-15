# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(interpreter-core)

add_definitions(-DRUN_ON_LINUX)
add_definitions(-Dattr_container_malloc=malloc)
add_definitions(-Dattr_container_free=free)

set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_APP_FRAMEWORK 1)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 0)

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

add_custom_command(TARGET interpreter-core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/*.wasm
    ${CMAKE_CURRENT_BINARY_DIR}/
    COMMENT "Copy test wasm files to build directory"
)

include(GoogleTest)
gtest_discover_tests(interpreter-core)
