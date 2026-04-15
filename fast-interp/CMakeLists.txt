# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(fast-interp-coverage)

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_FAST_INTERP 1)
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_FAST_JIT 0)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_MEMORY_PROFILING 0)
set(WAMR_BUILD_BULK_MEMORY 1)
set(WAMR_BUILD_REF_TYPES 1)
set(WAMR_BUILD_TAIL_CALL 1)

include(../../unit_common.cmake)

set(LLVM_SRC_ROOT "${WAMR_ROOT_DIR}/core/deps/llvm")
if (EXISTS "${LLVM_SRC_ROOT}/build")
    set(CMAKE_PREFIX_PATH "${LLVM_SRC_ROOT}/build;${CMAKE_PREFIX_PATH}")
    find_package(LLVM REQUIRED CONFIG)
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
list(FILTER source_all EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}/.*")

set(unit_test_sources
    ${source_all}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${UNCOMMON_SHARED_SOURCE}
)

add_executable(fast_interp_coverage_test ${unit_test_sources})
target_link_libraries(fast_interp_coverage_test ${LLVM_AVAILABLE_LIBS} gtest_main)

gtest_discover_tests(fast_interp_coverage_test)

# Copy wasm-apps from all opcode test directories
set(WASM_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../numeric/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../memory/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../control-flow/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../variable/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../table/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../constants/wasm-apps
    ${CMAKE_CURRENT_SOURCE_DIR}/../reference/wasm-apps
)

foreach(WASM_DIR ${WASM_DIRS})
    if(EXISTS ${WASM_DIR})
        get_filename_component(PARENT_DIR ${WASM_DIR} DIRECTORY)
        get_filename_component(CATEGORY ${PARENT_DIR} NAME)
        add_custom_command(TARGET fast_interp_coverage_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${CATEGORY}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${WASM_DIR} ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${CATEGORY}
            COMMENT "Copying ${CATEGORY} WASM files"
        )
    endif()
endforeach()
