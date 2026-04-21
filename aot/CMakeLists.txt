# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(aot)

add_definitions(-DRUN_ON_LINUX)
add_definitions(-Dattr_container_malloc=malloc)
add_definitions(-Dattr_container_free=free)
add_definitions(-DWASM_ENABLE_WAMR_COMPILER=1)
add_definitions(-DWASM_ENABLE_DUMP_CALL_STACK=1)
add_definitions(-DWASM_ENABLE_AOT_STACK_FRAME=1)

set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_APP_FRAMEWORK 1)
set(WAMR_BUILD_MULTI_MODULE 1)

include(../../unit_common.cmake)

set(LLVM_SRC_ROOT "${WAMR_ROOT_DIR}/core/deps/llvm")
if(NOT EXISTS "${LLVM_SRC_ROOT}/build")
  message(FATAL_ERROR "Cannot find LLVM dir: ${LLVM_SRC_ROOT}/build")
endif()
set(CMAKE_PREFIX_PATH "${LLVM_SRC_ROOT}/build;${CMAKE_PREFIX_PATH}")
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../common)

file(GLOB source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
    ${source_all}
    ${PLATFORM_SHARED_SOURCE}
    ${UTILS_SHARED_SOURCE}
    ${MEM_ALLOC_SHARED_SOURCE}
    ${NATIVE_INTERFACE_SOURCE}
    ${LIBC_BUILTIN_SOURCE}
    ${IWASM_COMMON_SOURCE}
    ${IWASM_INTERP_SOURCE}
    ${IWASM_AOT_SOURCE}
    ${IWASM_COMPL_SOURCE}
    ${WAMR_ROOT_DIR}/core/shared/utils/uncommon/bh_read_file.c
)

include(GoogleTest)

add_executable(aot ${unit_test_sources})

target_link_libraries(aot ${LLVM_AVAILABLE_LIBS} gtest_main gmock)

gtest_discover_tests(aot)

set(AOT_STATIC_FIXTURES
    aot_loader_test.aot aot_loader_test.wasm aot_loader_test.wat
    import_globals_test.aot import_globals_test.wasm import_globals_test.wat
    memory_operations_enhanced.aot memory_operations_enhanced.wasm memory_operations_enhanced.wat
    native_symbols_test.aot native_symbols_test.wasm native_symbols_test.wat
    table_init_test.aot table_init_test.wasm table_init_test.wat
)

list(APPEND AOT_STATIC_FIXTURES
    simple_function.wat
    multi_function.wat
    memory_operations.wat
)

set(_staged_files)
foreach(_fixture ${AOT_STATIC_FIXTURES})
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${_fixture}
            ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${_fixture}
            ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${_fixture}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${_fixture}
            ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${_fixture}
        COMMENT "Staging wasm-apps/${_fixture}"
    )
    list(APPEND _staged_files
        ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${_fixture}
        ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}
    )
endforeach()

add_custom_target(aot_copy_wasm_apps ALL DEPENDS ${_staged_files})

add_subdirectory(wasm-apps)

set(AOT_REGENERATED_FIXTURES
    simple_function.wasm simple_function.aot
    multi_function.wasm multi_function.aot
    memory_operations.wasm memory_operations.aot
)
if(TARGET aot_enhanced_wasm_files)
    set(_flat_mirrors)
    foreach(_fixture ${AOT_REGENERATED_FIXTURES})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.stamp
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${_fixture}
                ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}
            COMMAND ${CMAKE_COMMAND} -E touch
                ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.stamp
            DEPENDS aot_enhanced_wasm_files
            COMMENT "Mirroring regenerated wasm-apps/${_fixture} to build root"
        )
        list(APPEND _flat_mirrors ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.stamp)
    endforeach()
    add_custom_target(aot_mirror_regenerated ALL DEPENDS ${_flat_mirrors})
endif()

add_dependencies(aot aot_copy_wasm_apps)
if(TARGET aot_enhanced_wasm_files)
    add_dependencies(aot aot_enhanced_wasm_files)
endif()
if(TARGET aot_mirror_regenerated)
    add_dependencies(aot aot_mirror_regenerated)
endif()
