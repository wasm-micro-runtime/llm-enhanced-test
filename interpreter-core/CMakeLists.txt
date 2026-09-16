# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(interpreter-core MODES classic-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# No feature switch is needed: the cases that import malloc/free register
# their own native under "env"
# (interpreter_coverage_improve_step_3.cc), so libc-builtin stays off.

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
    ${source_all}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${WAMR_ROOT_DIR}/core/shared/utils/uncommon/bh_read_file.c
)

add_executable(interpreter-core ${unit_test_sources})

target_link_libraries(interpreter-core
    gtest_main
    ${LLVM_AVAILABLE_LIBS}
)

# The running mode (WASM_ENABLE_INTERP / WASM_ENABLE_FAST_INTERP), the libc
# and bulk-memory macros are supplied by config_common.cmake for this
# directory, so the suite does not restate them on the target.

gtest_discover_tests(interpreter-core)

# Generated WASM fixture declarations

set(_generated_wasm_fixtures
    arithmetic_bitwise
    complex_sections_module
    datacount_test
    empty_table_segments
    floating_point_operations
    function_invocation_test
    import_export_module
    invalid_magic_number
    multiple_table_types
    simd_shuffle_test
    table_validation_test
    valid_basic_module
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET interpreter-core
        SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${_fixture}.wat
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_fixture}.wasm
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture)
