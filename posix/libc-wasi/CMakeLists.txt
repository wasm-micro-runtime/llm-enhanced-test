# Copyright (C) 2024 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

wamr_unit_test_suite_run_modes(posix-libc-wasi MODES classic-interp fast-interp aot)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# platform_api_posix.cmake only compiles posix_file.c / posix_clock.c /
# posix_socket.c (plus the libc-util sources) when WAMR_BUILD_LIBC_WASI=1,
# and these cases call into them.  Neither WAMR_BUILD_LIBC_BUILTIN nor
# WAMR_BUILD_SHARED_MEMORY is needed here.

set(WAMR_BUILD_LIBC_WASI 1)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../unit_common.cmake)

# TODO: IWASM_COMPL_SOURCE is not added to any target below; drop this include
# once the AOT-compiler dependency of these cases has been re-checked.
include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# TODO: no case here references bh_read_file/bh_getopt; re-check whether
# UNCOMMON_SHARED_SOURCE is still needed before keeping it.
set(unit_test_sources
    ${source_all}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${UNCOMMON_SHARED_SOURCE}
)

add_executable(llm-enhanced-test-posix-libc-wasi ${unit_test_sources})

target_link_libraries(llm-enhanced-test-posix-libc-wasi ${LLVM_AVAILABLE_LIBS} gtest_main -lm -ldl -lpthread)

gtest_discover_tests(llm-enhanced-test-posix-libc-wasi
    PROPERTIES TIMEOUT 30
)
