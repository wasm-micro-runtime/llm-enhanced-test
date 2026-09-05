# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(aot MODES aot)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

add_definitions(-DWASM_ENABLE_WAMR_COMPILER=1)

set(WAMR_BUILD_DUMP_CALL_STACK 1)
set(WAMR_BUILD_LIBC_WASI 0)

include(../../unit_common.cmake)

include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${IWASM_DIR}/interpreter)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../common)

file(GLOB source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
    ${source_all}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${IWASM_INTERP_SOURCE}
    ${IWASM_COMPL_SOURCE}
    ${UNCOMMON_SHARED_SOURCE}
)

include(GoogleTest)

add_executable(aot ${unit_test_sources})

target_link_libraries(aot ${LLVM_AVAILABLE_LIBS} gtest_main gmock)

gtest_discover_tests(aot)

set(AOT_WAT_FIXTURES
    aot_loader_test
    import_globals_test
    memory_operations
    memory_operations_enhanced
    multi_function
    native_symbols_test
    simple_function
    table_init_test
)

set(_aot_flat_mirrors)
foreach(_fixture ${AOT_WAT_FIXTURES})
    set(_wasm_output ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${_fixture}.wasm)
    set(_aot_output ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${_fixture}.aot)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET aot
        SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${_fixture}.wat
        OUTPUT ${_wasm_output}
    )
    wamr_unit_test_compile_wasm_to_aot(
        TARGET aot
        INPUT ${_wasm_output}
        OUTPUT ${_aot_output}
    )

    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.wasm
            ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.aot
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${_wasm_output}
            ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.wasm
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${_aot_output}
            ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.aot
        DEPENDS ${_wasm_output} ${_aot_output}
        COMMENT "Mirroring generated ${_fixture} wasm/aot to build root"
    )
    list(APPEND _aot_flat_mirrors
        ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.wasm
        ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.aot
    )
endforeach()

add_custom_target(aot_mirror_generated ALL DEPENDS ${_aot_flat_mirrors})
add_dependencies(aot aot_mirror_generated)

unset(_aot_flat_mirrors)
unset(_fixture)
unset(_wasm_output)
unset(_aot_output)
