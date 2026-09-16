# Copyright (C) 2025 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(ssp MODES classic-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# Feature under test: the sandboxed-system-primitives cases are gated on
# WASM_ENABLE_LIBC_WASI, which config_common.cmake / libc_wasi.cmake already
# define for this directory.
set(WAMR_BUILD_LIBC_WASI 1)

include(../../unit_common.cmake)

# The test sources pick their platform cases with BUILD_TARGET_*; the host
# macros are not something the build scripts derive, so they are defined here.
# The WASM_ENABLE_* macros the sources test are already defined for this
# directory by config_common.cmake and must not be restated per target.
if(WAMR_BUILD_TARGET MATCHES "X86_.*")
  add_definitions(-DBUILD_TARGET_X86=1)
elseif(WAMR_BUILD_TARGET MATCHES "AARCH64.*")
  add_definitions(-DBUILD_TARGET_AARCH64=1)
elseif(WAMR_BUILD_TARGET MATCHES "RISCV.*")
  add_definitions(-DBUILD_TARGET_RISCV=1)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${WAMR_ROOT_DIR}/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/include)
include_directories(${WAMR_ROOT_DIR}/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/src)

set(unit_test_sources
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
)

set(_ssp_targets
  ssp_unified_test
  ssp_step_1_test
  ssp_step_2_test
  ssp_step_3_test
  enhanced_blocking_op_test
  enhanced_posix_test
)
foreach(_target IN LISTS _ssp_targets)
  add_executable(${_target} ${_target}.cc ${unit_test_sources})
  target_link_libraries(${_target} gtest_main)
  gtest_discover_tests(${_target})
endforeach()
unset(_ssp_targets)
unset(_target)
