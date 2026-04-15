# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(runtime-common-wasi-mem64)

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_SHARED_MEMORY 1)
set(WAMR_BUILD_MEMORY64 1)
set(WAMR_BUILD_LIB_PTHREAD 1)
set(WAMR_BUILD_THREAD_MGR 1)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 1)

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${WAMR_ROOT_DIR}/tests/unit/common)

set(UNIT_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime_common_enhanced_test.cc
)

set(unit_test_sources
  ${UNIT_SOURCE}
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
)

add_executable(runtime-common-wasi-mem64 ${unit_test_sources})

target_link_libraries(runtime-common-wasi-mem64 gtest_main)

include(GoogleTest)
gtest_discover_tests(runtime-common-wasi-mem64)