# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(shared-utils MODES classic-interp aot)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

set(WAMR_BUILD_LIBC_WASI 0)

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Runtime timer enhanced test
set(RUNTIME_TIMER_ENHANCED_SOURCE
    runtime_timer_enhanced_test.cc
)

# Memory utility functions test (Step 2)
set(MEMORY_UTILITY_STEP2_SOURCE
    shared_utils_add_30_step_2.cc
)

# Assertion and File I/O functions test (Step 3)
set(ASSERTION_FILEIO_STEP3_SOURCE
    shared_utils_add_30_step_3.cc
)

set(unit_test_sources
    ${RUNTIME_TIMER_ENHANCED_SOURCE}
    ${WAMR_RUNTIME_LIB_SOURCE}
)

set(memory_util_test_sources
    ${MEMORY_UTILITY_STEP2_SOURCE}
    ${WAMR_RUNTIME_LIB_SOURCE}
)

set(assertion_fileio_test_sources
    ${ASSERTION_FILEIO_STEP3_SOURCE}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${WAMR_ROOT_DIR}/core/shared/utils/uncommon/bh_read_file.c
)

add_executable(runtime_timer_enhanced_test ${unit_test_sources})
add_executable(shared_utils_add_30_step_2 ${memory_util_test_sources})
add_executable(shared_utils_add_30_step_3 ${assertion_fileio_test_sources})

target_link_libraries(runtime_timer_enhanced_test gtest_main)
target_link_libraries(shared_utils_add_30_step_2 gtest_main)
target_link_libraries(shared_utils_add_30_step_3 gtest_main)

gtest_discover_tests(runtime_timer_enhanced_test)
gtest_discover_tests(shared_utils_add_30_step_2)
gtest_discover_tests(shared_utils_add_30_step_3)
