# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(wasm-vm MODES classic-interp fast-interp aot)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(unit_test_sources
    wasm_vm_runtime_lifecycle_test.cc
    ${PLATFORM_SHARED_SOURCE}
    ${UTILS_SHARED_SOURCE}
    ${MEM_ALLOC_SHARED_SOURCE}
    ${LIBC_BUILTIN_SOURCE}
    ${LIBC_WASI_SOURCE}
    ${IWASM_COMMON_SOURCE}
    ${IWASM_INTERP_SOURCE}
    ${IWASM_AOT_SOURCE}
    ${IWASM_COMPL_SOURCE}
    ${UNCOMMON_SHARED_SOURCE}
)

add_executable(wasm_vm_runtime_lifecycle_test ${unit_test_sources})

target_link_libraries(wasm_vm_runtime_lifecycle_test ${LLVM_AVAILABLE_LIBS} gtest_main)

gtest_discover_tests(wasm_vm_runtime_lifecycle_test)
