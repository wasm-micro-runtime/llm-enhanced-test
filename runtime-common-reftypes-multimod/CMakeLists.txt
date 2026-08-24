# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(runtime-common-reftypes-multimod)

wamr_unit_test_suite_run_modes(runtime-common-reftypes-multimod MODES none)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_REF_TYPES 1)
set(WAMR_BUILD_THREAD_MGR 1)
set(WAMR_BUILD_SHARED_MEMORY 1)
set(WAMR_BUILD_MULTI_MODULE 1)

add_definitions(-DWASM_ENABLE_MEMORY_PROFILING=1)
add_definitions(-DWASM_ENABLE_MEMORY_TRACING=1)

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${IWASM_DIR}/interpreter)

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# Exclude build directory from sources
foreach(source ${source_all})
    string(FIND ${source} "/build/" build_pos)
    if(build_pos EQUAL -1)
        list(APPEND UNIT_SOURCE ${source})
    endif()
endforeach()

aux_source_directory(. SRC_LIST)

set(unit_test_sources
  ${UNIT_SOURCE}
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
  ${SRC_LIST}
)

add_executable(runtime-common-reftypes-multimod ${unit_test_sources})

target_link_libraries(runtime-common-reftypes-multimod ${LLVM_AVAILABLE_LIBS} gtest_main gmock)

wamr_unit_test_copy_wasm_files(runtime-common-reftypes-multimod
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/wasm-apps/main.wasm
  DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Copy main.wasm to the directory: build/runtime-common."
)

wamr_unit_test_compile_wasm_to_aot(
  TARGET runtime-common-reftypes-multimod
  INPUT ${CMAKE_CURRENT_LIST_DIR}/wasm-apps/main.wasm
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main.aot
)

# Skip gtest_discover_tests for now to avoid build issues
# include(GoogleTest)
gtest_discover_tests(runtime-common-reftypes-multimod)
