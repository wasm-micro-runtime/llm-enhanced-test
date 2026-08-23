# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project (interpreter-invoke)

wamr_unit_test_suite_run_modes(interpreter-invoke MODES classic-interp fast-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions (-DRUN_ON_LINUX)

add_definitions (-Dattr_container_malloc=malloc)
add_definitions (-Dattr_container_free=free)
# add_definitions (-DWASM_ENABLE_WAMR_COMPILER=1)

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

file(GLOB INTERPRETER_INVOKE_WASM_FILES
    ${CMAKE_CURRENT_LIST_DIR}/wasm-apps/*.wasm
)
wamr_unit_test_copy_wasm_files(interpreter-invoke-classic
    FILES ${INTERPRETER_INVOKE_WASM_FILES}
    DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copy test wasm files to the directory of google test"
)

gtest_discover_tests(interpreter-invoke-classic)
gtest_discover_tests(interpreter-invoke-fast)
