# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(aot-arch MODES aot)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# The cases drive the AOT relocation code directly; no feature switch is
# needed (the probe builds and passes all 3 cases without libc-builtin).

include(../../unit_common.cmake)

# The test source picks its relocation cases with BUILD_TARGET_*; the macros
# naming the host are not something config_common.cmake derives, so they are
# defined here.  Everything else it needs to know (WASM_ENABLE_SIMD,
# WASM_ENABLE_AOT, WASM_ENABLE_JIT, WASM_ENABLE_MEMORY64, ...) is already
# added to this directory by config_common.cmake and must not be restated.
if(WAMR_BUILD_TARGET MATCHES "X86_.*")
  add_definitions(-DBUILD_TARGET_X86_64=1)
elseif(WAMR_BUILD_TARGET MATCHES "AARCH64.*")
  add_definitions(-DBUILD_TARGET_AARCH64=1)
elseif(WAMR_BUILD_TARGET MATCHES "RISCV64.*")
  add_definitions(-DBUILD_TARGET_RISCV64_LP64D=1)
elseif(WAMR_BUILD_TARGET MATCHES "RISCV32.*")
  add_definitions(-DBUILD_TARGET_RISCV32_ILP32=1)
endif()

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
  ${source_all}
  ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable(aot_arch_tests ${unit_test_sources})
target_link_libraries(aot_arch_tests ${LLVM_AVAILABLE_LIBS} gtest_main)
gtest_discover_tests(aot_arch_tests
    PROPERTIES TIMEOUT 300
)
