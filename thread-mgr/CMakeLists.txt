# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(thread-mgr MODES classic-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# The thread manager and its pthread wrapper are the features under test;
# the enhanced case also calls wasm_cluster_set_context(), which needs the
# module instance context.  config_common.cmake turns these into the
# WASM_ENABLE_* macros for the whole directory, so no target repeats them.
set(WAMR_BUILD_THREAD_MGR 1)
set(WAMR_BUILD_LIB_PTHREAD 1)
set(WAMR_BUILD_MODULE_INST_CONTEXT 1)

include(../../unit_common.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${WAMR_ROOT_DIR}/core/iwasm/libraries/thread-mgr)

set(unit_test_sources
  ${WAMR_RUNTIME_LIB_SOURCE}
)

set(_thread_mgr_targets
  thread_mgr_add_25_step_2
  thread_mgr_add_25_step_3
  enhanced_thread_manager_test
)
foreach(_target IN LISTS _thread_mgr_targets)
  add_executable(${_target} ${_target}.cc ${unit_test_sources})
  target_link_libraries(${_target}
    gtest_main
    pthread
    ${LLVM_AVAILABLE_LIBS}
    ${UV_A_LIBS}
  )
  gtest_discover_tests(${_target})
endforeach()
unset(_thread_mgr_targets)
unset(_target)
