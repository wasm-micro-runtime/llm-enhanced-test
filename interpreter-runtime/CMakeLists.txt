# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project (interpreter-runtime)

add_definitions (-DRUN_ON_LINUX)

add_definitions (-Dattr_container_malloc=malloc)
add_definitions (-Dattr_container_free=free)
# add_definitions (-DWASM_ENABLE_WAMR_COMPILER=1)

set (WAMR_BUILD_LIBC_WASI 1)
set (WAMR_BUILD_APP_FRAMEWORK 1)
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_AOT 0)
set (WAMR_BUILD_THREAD_MGR 1)
set (WAMR_BUILD_MULTI_MODULE 1)
set (WAMR_BUILD_BULK_MEMORY 1)
set (WAMR_BUILD_DUMP_CALL_STACK 1)

include (../../unit_common.cmake)

# Include LIBC WASI if enabled
if (WAMR_BUILD_LIBC_WASI EQUAL 1)
    include (${CMAKE_CURRENT_SOURCE_DIR}/../../../../core/iwasm/libraries/libc-wasi/libc_wasi.cmake)
endif ()

# Include thread manager if enabled
if (WAMR_BUILD_THREAD_MGR EQUAL 1)
    include (${CMAKE_CURRENT_SOURCE_DIR}/../../../../core/iwasm/libraries/thread-mgr/thread_mgr.cmake)
endif ()

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

file (GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set (UNIT_SOURCE ${source_all})

set (unit_test_sources
     ${UNIT_SOURCE}
     ${PLATFORM_SHARED_SOURCE}
     ${UTILS_SHARED_SOURCE}
     ${MEM_ALLOC_SHARED_SOURCE}
     ${NATIVE_INTERFACE_SOURCE}
     ${LIBC_BUILTIN_SOURCE}
     ${LIBC_WASI_SOURCE}
     ${IWASM_COMMON_SOURCE}
     ${IWASM_INTERP_SOURCE}
     ${THREAD_MGR_SOURCE}
    )

# Now simply link against gtest or gtest_main as needed. Eg
add_executable (interpreter-runtime ${unit_test_sources})

target_link_libraries (interpreter-runtime ${LLVM_AVAILABLE_LIBS} gtest_main )

gtest_discover_tests(interpreter-runtime)
