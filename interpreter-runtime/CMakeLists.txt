# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(interpreter-runtime MODES classic-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# add_definitions (-DWASM_ENABLE_WAMR_COMPILER=1)

set (WAMR_BUILD_LIBC_WASI 1)
set (WAMR_BUILD_THREAD_MGR 1)
set (WAMR_BUILD_MULTI_MODULE 1)
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
