# Copyright (C) 2024 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(llm-enhanced-test-posix)

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_PLATFORM "linux")
set(WAMR_BUILD_TARGET "X86_64")
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_FAST_INTERP 1)
set(WAMR_BUILD_SHARED_MEMORY 1)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../unit_common.cmake)
include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../common)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)

# =============================================================================
# Target 1: llm-enhanced-test-posix-1
# Socket options, multicast/TTL, and POSIX coverage improvement tests.
# Requires LLVM linking for AOT compilation support.
# =============================================================================
set(POSIX_1_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_socket_options_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_multicast_ttl_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_coverage_improve_step_1.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_coverage_improve_step_2.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_coverage_improve_step_3.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_coverage_improve_step_4.cc
)

add_executable(llm-enhanced-test-posix-1
    ${POSIX_1_SOURCES}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${UNCOMMON_SHARED_SOURCE}
)

target_link_libraries(llm-enhanced-test-posix-1 ${LLVM_AVAILABLE_LIBS} gtest_main -lm -ldl -lpthread)

gtest_discover_tests(llm-enhanced-test-posix-1
    PROPERTIES TIMEOUT 30
)

# =============================================================================
# Target 2: llm-enhanced-test-posix-2
# POSIX platform function tests (clock, sleep, time, malloc, file, socket,
# blocking ops, thread). Tests platform API directly without LLVM.
# =============================================================================
set(POSIX_PLATFORM_DIR ${WAMR_ROOT_DIR}/core/shared/platform/common/posix)

set(POSIX_2_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_clock_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_time_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_malloc_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_file_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_socket_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_posix_thread_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/enhanced_posix_file_test.cc
)

set(POSIX_2_PLATFORM_SOURCES
    ${POSIX_PLATFORM_DIR}/posix_clock.c
    ${POSIX_PLATFORM_DIR}/posix_sleep.c
    ${POSIX_PLATFORM_DIR}/posix_time.c
    ${POSIX_PLATFORM_DIR}/posix_malloc.c
    ${POSIX_PLATFORM_DIR}/posix_file.c
    ${POSIX_PLATFORM_DIR}/posix_socket.c
    ${POSIX_PLATFORM_DIR}/posix_blocking_op.c
    ${POSIX_PLATFORM_DIR}/posix_thread.c
)

add_executable(llm-enhanced-test-posix-2
    ${POSIX_2_SOURCES}
    ${POSIX_2_PLATFORM_SOURCES}
    ${WAMR_RUNTIME_LIB_SOURCE}
)

target_link_libraries(llm-enhanced-test-posix-2 gtest_main -lm -ldl -lpthread)

gtest_discover_tests(llm-enhanced-test-posix-2)
