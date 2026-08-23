# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project (compilation-jit)

wamr_unit_test_suite_run_modes(compilation-jit MODES llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions (-DRUN_ON_LINUX)

add_definitions (-Dattr_container_malloc=malloc)
add_definitions (-Dattr_container_free=free)
add_definitions (-DWASM_ENABLE_WAMR_COMPILER=1)
add_definitions (-DWASM_ENABLE_DUMP_CALL_STACK=1)
add_definitions (-DWASM_ENABLE_AOT_STACK_FRAME=1)

# Set suite-specific WAMR configuration before including unit_common.cmake.
set (WAMR_BUILD_PLATFORM "linux")
set (WAMR_BUILD_TARGET "X86_64")
set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_APP_FRAMEWORK 0)
set (WAMR_BUILD_THREAD_MGR 1)

# The SIMD advanced compilation tests load `simd_test.wasm` which contains v128
# value types. So we must enable SIMD support in the runtime loader/compiler.
set (WAMR_BUILD_SIMD 1)

include (../../unit_common.cmake)

set (LLVM_SRC_ROOT "${WAMR_ROOT_DIR}/core/deps/llvm")
if (NOT EXISTS "${LLVM_SRC_ROOT}/build")
  message (FATAL_ERROR "Cannot find LLVM dir: ${LLVM_SRC_ROOT}/build")
endif ()
set (CMAKE_PREFIX_PATH "${LLVM_SRC_ROOT}/build;${CMAKE_PREFIX_PATH}")
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include (${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

file (GLOB source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set (UNIT_SOURCE ${source_all})

set (unit_test_sources
     ${UNIT_SOURCE}
     ${WAMR_RUNTIME_LIB_SOURCE}
     ${UNCOMMON_SHARED_SOURCE}
     ${SRC_LIST}
     ${PLATFORM_SHARED_SOURCE}
     ${UTILS_SHARED_SOURCE}
     ${MEM_ALLOC_SHARED_SOURCE}
     ${LIB_HOST_AGENT_SOURCE}
     ${NATIVE_INTERFACE_SOURCE}
     ${LIBC_BUILTIN_SOURCE}
     ${IWASM_COMMON_SOURCE}
     ${IWASM_INTERP_SOURCE}
     ${IWASM_AOT_SOURCE}
     ${IWASM_COMPL_SOURCE}
     ${WASM_APP_LIB_SOURCE_ALL}
    )

# Now simply link against gtest or gtest_main as needed. Eg
add_executable (compilation-jit ${unit_test_sources})

target_link_libraries (compilation-jit ${LLVM_AVAILABLE_LIBS} gtest_main )

wamr_unit_test_copy_wasm_files(compilation-jit
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/wasm-apps/main.wasm
  DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Copy WASM test files to the directory: build/compilation-jit."
)

include(GoogleTest)
gtest_discover_tests(compilation-jit)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/llvm_test.wat|${CMAKE_CURRENT_BINARY_DIR}/llvm_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/simd_test.wat|${CMAKE_CURRENT_BINARY_DIR}/simd_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET compilation-jit
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
