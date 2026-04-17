# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project (compilation-emit)

add_definitions (-DRUN_ON_LINUX)

add_definitions (-Dattr_container_malloc=malloc)
add_definitions (-Dattr_container_free=free)
add_definitions (-DWASM_ENABLE_WAMR_COMPILER=1)
add_definitions (-DWASM_ENABLE_DUMP_CALL_STACK=1)
add_definitions (-DWASM_ENABLE_AOT_STACK_FRAME=1)

set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_APP_FRAMEWORK 0)
set (WAMR_BUILD_THREAD_MGR 1)
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_AOT 1)
set (WAMR_BUILD_GC 1)
set (WAMR_BUILD_REF_TYPES 1)

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

file (GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

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
add_executable (compilation-emit ${unit_test_sources})

target_link_libraries (compilation-emit ${LLVM_AVAILABLE_LIBS} gtest_main )


# Compile main.wat -> main.wasm if wat2wasm is available
find_program(WAT2WASM wat2wasm)
if(WAT2WASM)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/main.wasm
        COMMAND ${WAT2WASM} ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/main.wat
                -o ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/main.wasm
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/main.wat
        COMMENT "Compiling main.wat to main.wasm"
    )
    add_custom_target(compilation-emit-main-wasm
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/main.wasm
    )
    add_dependencies(compilation-emit compilation-emit-main-wasm)
else()
    message(WARNING "wat2wasm not found, main.wasm will not be compiled from source")
endif()

add_custom_command(TARGET compilation-emit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/*.wasm
        ${CMAKE_CURRENT_BINARY_DIR}/
        COMMENT "Copy test wasm files to the directory of google test"
        )

include(GoogleTest)
# NOTE: `compilation-emit` is a single gtest binary, and CTest runs each
# discovered test as a separate process with a gtest filter. When code coverage
# is enabled, running these processes concurrently can race on `.gcda` writes and
# cause sporadic "Subprocess aborted". Force serialization for this executable's
# tests to make `ctest --parallel` stable.
gtest_discover_tests(compilation-emit
                     PROPERTIES RUN_SERIAL TRUE)