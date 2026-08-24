# CMakeLists.txt for SIMD extension opcode unit tests
# Builds comprehensive test suite for SIMD opcodes including i8x16.splat, i16x8.splat, i32x4.splat, f64x2.extmul_high_i32x4_s, f64x2.extmul_high_i32x4_u, i16x8.extmul_high_i8x16_u
# with SIMD extension support and cross-execution mode validation

cmake_minimum_required(VERSION 3.14)

project(test-enhanced-extension-opcodes)

wamr_unit_test_suite_run_modes(extension MODES llvm-jit)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

# Enable required WAMR build features for SIMD opcode testing (i8x16.splat, i16x8.splat, i32x4.splat, f64x2.extmul_high_i32x4_s, f64x2.extmul_high_i32x4_u, i16x8.extmul_high_i8x16_u)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_MEMORY_PROFILING 0)
set(WAMR_BUILD_SIMD 1)

# Platform-specific definitions
add_definitions(-DRUN_ON_LINUX)

# Include shared unit test configuration
include(../../unit_common.cmake)

# Include AOT compilation support
include(${IWASM_DIR}/compilation/iwasm_compl.cmake)

# Set up include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Collect all test source files
file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
# Remove files from build directory to avoid conflicts
list(FILTER source_all EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}/.*")

set(UNIT_SOURCE ${source_all})

# Prepare unit test sources
set(unit_test_sources
        ${UNIT_SOURCE}
        ${WAMR_RUNTIME_LIB_SOURCE}
        ${UNCOMMON_SHARED_SOURCE}
        )

# Create test executables for each SIMD opcode
add_executable(enhanced_i8x16_splat_test enhanced_i8x16_splat_test.cc ${WAMR_RUNTIME_LIB_SOURCE} ${UNCOMMON_SHARED_SOURCE})
add_executable(enhanced_i16x8_splat_test enhanced_i16x8_splat_test.cc ${WAMR_RUNTIME_LIB_SOURCE} ${UNCOMMON_SHARED_SOURCE})
add_executable(enhanced_f64x2_extmul_high_i32x4_s_test enhanced_f64x2_extmul_high_i32x4_s_test.cc ${WAMR_RUNTIME_LIB_SOURCE} ${UNCOMMON_SHARED_SOURCE})
add_executable(enhanced_f64x2_extmul_high_i32x4_u_test enhanced_f64x2_extmul_high_i32x4_u_test.cc ${WAMR_RUNTIME_LIB_SOURCE} ${UNCOMMON_SHARED_SOURCE})
add_executable(enhanced_i16x8_extmul_high_i8x16_u_test enhanced_i16x8_extmul_high_i8x16_u_test.cc ${WAMR_RUNTIME_LIB_SOURCE} ${UNCOMMON_SHARED_SOURCE})

# Link required libraries
target_link_libraries(enhanced_i8x16_splat_test ${LLVM_AVAILABLE_LIBS} gtest_main)
target_link_libraries(enhanced_i16x8_splat_test ${LLVM_AVAILABLE_LIBS} gtest_main)
target_link_libraries(enhanced_f64x2_extmul_high_i32x4_s_test ${LLVM_AVAILABLE_LIBS} gtest_main)
target_link_libraries(enhanced_f64x2_extmul_high_i32x4_u_test ${LLVM_AVAILABLE_LIBS} gtest_main)
target_link_libraries(enhanced_i16x8_extmul_high_i8x16_u_test ${LLVM_AVAILABLE_LIBS} gtest_main)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(enhanced_i8x16_splat_test)
gtest_discover_tests(enhanced_i16x8_splat_test)
gtest_discover_tests(enhanced_f64x2_extmul_high_i32x4_s_test)
gtest_discover_tests(enhanced_f64x2_extmul_high_i32x4_u_test)
gtest_discover_tests(enhanced_i16x8_extmul_high_i8x16_u_test)

wamr_unit_test_add_wasm_copy_target(copy_extension_wasm_apps
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
    DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMENT "Copying SIMD WASM test files to build directory"
)

# Make all test executables depend on the copy operation
add_dependencies(enhanced_i8x16_splat_test copy_extension_wasm_apps)
add_dependencies(enhanced_i16x8_splat_test copy_extension_wasm_apps)
add_dependencies(enhanced_f64x2_extmul_high_i32x4_s_test copy_extension_wasm_apps)
add_dependencies(enhanced_f64x2_extmul_high_i32x4_u_test copy_extension_wasm_apps)
add_dependencies(enhanced_i16x8_extmul_high_i8x16_u_test copy_extension_wasm_apps)

# Generated WASM fixture declarations

# Generate tracked WASM fixtures from WAT/WAST sources.
set(_generated_wasm_fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_extmul_high_i32x4_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_extmul_high_i32x4_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_extmul_high_i32x4_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_extmul_high_i32x4_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/f64x2_splat_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/f64x2_splat_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i16x8_extmul_high_i8x16_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i16x8_extmul_high_i8x16_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i16x8_splat_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i16x8_splat_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_splat_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_splat_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i32x4_splat_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i32x4_splat_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64x2_splat_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64x2_splat_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i64x2_splat_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i64x2_splat_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_lt_s_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_lt_s_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_lt_s_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_lt_s_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_lt_u_stack_underflow.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_lt_u_stack_underflow.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_lt_u_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_lt_u_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/i8x16_splat_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/i8x16_splat_test.wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/v128_load_test.wat|${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/v128_load_test.wasm
)
foreach(_fixture IN LISTS _generated_wasm_fixtures)
    string(REPLACE "|" ";" _fixture_parts "${_fixture}")
    list(GET _fixture_parts 0 _fixture_source)
    list(GET _fixture_parts 1 _fixture_output)
    wamr_unit_test_compile_wat_to_wasm(
        TARGET copy_extension_wasm_apps
        SOURCE ${_fixture_source}
        OUTPUT ${_fixture_output}
    )
endforeach()
unset(_generated_wasm_fixtures)
unset(_fixture_parts)
unset(_fixture_source)
unset(_fixture_output)
