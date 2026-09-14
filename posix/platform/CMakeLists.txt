# Copyright (C) 2024 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(posix-platform MODES classic-interp fast-interp aot)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# No WAMR_BUILD_* switch: these cases only call functions from the platform
# sources that are always compiled (posix_time.c, posix_malloc.c,
# posix_thread.c).  See ../CMakeLists.txt for the full mapping.

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
    ${source_all}
    ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable(llm-enhanced-test-posix-platform ${unit_test_sources})

target_link_libraries(llm-enhanced-test-posix-platform gtest_main -lm -ldl -lpthread)

gtest_discover_tests(llm-enhanced-test-posix-platform
    PROPERTIES TIMEOUT 30
)
