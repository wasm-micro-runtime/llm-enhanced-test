# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project (interpreter-invoke)

add_definitions (-DRUN_ON_LINUX)

add_definitions (-Dattr_container_malloc=malloc)
add_definitions (-Dattr_container_free=free)
# add_definitions (-DWASM_ENABLE_WAMR_COMPILER=1)

set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_JIT 0)
set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_APP_FRAMEWORK 1)

include (../../unit_common.cmake)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

file (GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set (UNIT_SOURCE ${source_all})

set (unit_test_sources
  ${UNIT_SOURCE}
  ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable (interpreter-invoke-classic ${unit_test_sources})
target_compile_definitions(interpreter-invoke-classic PUBLIC -DWAMR_BUILD_FAST_INTERP=0)
target_link_libraries (interpreter-invoke-classic gtest_main)

add_executable (interpreter-invoke-fast ${unit_test_sources})
target_compile_definitions(interpreter-invoke-fast PUBLIC -DWAMR_BUILD_FAST_INTERP=1)
target_link_libraries (interpreter-invoke-fast gtest_main)

# Copy WASM files to build directory for classic interpreter
# fast interpreter uses the same WASM files
add_custom_command(TARGET interpreter-invoke-classic POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_LIST_DIR}/wasm-apps/*.wasm
    ${CMAKE_CURRENT_BINARY_DIR}/
    COMMENT "Copy test wasm files to the directory of google test"
)

gtest_discover_tests(interpreter-invoke-classic)
gtest_discover_tests(interpreter-invoke-fast)
