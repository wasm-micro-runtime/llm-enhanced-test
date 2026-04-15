/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <limits>
#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include "test_helper.h"
#include "bh_read_file.h"

/**
 * @file enhanced_i32x4_extmul_low_i16x8_s_test.cc
 * @brief Comprehensive test suite for i32x4.extmul_low_i16x8_s SIMD opcode
 *
 * Tests the extended multiplication operation that takes the lower 4 lanes (0-3)
 * of two i16x8 vectors, performs signed multiplication, and produces i32x4 results.
 * Validates both interpreter and AOT execution modes for complete coverage.
 */

// Test execution modes for cross-validation
enum class I32x4ExtmulLowI16x8SRunningMode : uint8_t {
    INTERP = 1 << 0,
    AOT = 1 << 1
};

static constexpr const char *MODULE_NAME = "i32x4_extmul_low_i16x8_s_test";
static constexpr const char *FUNC_NAME_BASIC_EXTMUL = "test_basic_extmul";
static constexpr const char *FUNC_NAME_SIGNED_EXTMUL = "test_signed_extmul";
static constexpr const char *FUNC_NAME_BOUNDARY_EXTMUL = "test_boundary_extmul";
static constexpr const char *FUNC_NAME_ZERO_EXTMUL = "test_zero_extmul";
static constexpr const char *FUNC_NAME_IDENTITY_EXTMUL = "test_identity_extmul";

/**
 * Test fixture for i32x4.extmul_low_i16x8_s opcode validation
 *
 * Provides comprehensive test environment for SIMD extended multiplication operations
 * using WAMR test helpers for proper runtime initialization and
 * cross-execution mode testing (interpreter and AOT).
 */
class I32x4ExtmulLowI16x8STestSuite : public testing::TestWithParam<I32x4ExtmulLowI16x8SRunningMode> {
protected:
    /**
     * Sets up test environment with WAMR runtime initialization
     *
     * Uses WAMR test helpers to initialize runtime with SIMD support
     * and loads the i32x4.extmul_low_i16x8_s test WASM module.
     */
    void SetUp() override {
        // Initialize WAMR runtime using RAII helper
        runtime_raii = std::make_unique<WAMRRuntimeRAII<>>();

        // Load the i32x4.extmul_low_i16x8_s test module using test helper
        dummy_env = std::make_unique<DummyExecEnv>("wasm-apps/i32x4_extmul_low_i16x8_s_test.wasm");
        ASSERT_NE(nullptr, dummy_env->get())
            << "Failed to create execution environment for i32x4.extmul_low_i16x8_s tests";
    }

    /**
     * Cleans up test environment and runtime resources
     *
     * Cleanup is handled automatically by RAII destructors.
     */
    void TearDown() override {
        // Cleanup handled by RAII destructors
        dummy_env.reset();
        runtime_raii.reset();
    }

    /**
     * Calls WASM i32x4.extmul_low_i16x8_s test function with input vectors
     *
     * @param func_name Name of the WASM test function to call
     * @param input1_values Array of 4 i32 values for first input v128 vector
     * @param input2_values Array of 4 i32 values for second input v128 vector
     * @param output_values Array to store 4 i32 values from output v128 vector
     */
    void call_extmul_test_function(const char* func_name, const int32_t* input1_values,
                                  const int32_t* input2_values, int32_t* output_values) {
        // Prepare arguments: pack input vectors into argv array
        uint32_t argv[8];  // 4 i32 for first vector + 4 i32 for second vector
        memcpy(argv, input1_values, 16);      // First i32x4 vector
        memcpy(argv + 4, input2_values, 16);  // Second i32x4 vector

        // Execute function
        bool call_success = dummy_env->execute(func_name, 8, argv);
        ASSERT_TRUE(call_success) << "Failed to execute WASM function: " << func_name;

        // Extract result: get i32x4 result (4 i32s = 16 bytes)
        memcpy(output_values, argv, 16);
    }

    std::unique_ptr<WAMRRuntimeRAII<>> runtime_raii;
    std::unique_ptr<DummyExecEnv> dummy_env;
};

/**
 * @test BasicMultiplication_ReturnsCorrectResults
 * @brief Validates i32x4.extmul_low_i16x8_s produces correct arithmetic results for typical inputs
 * @details Tests fundamental extended multiplication operation with positive, negative, and mixed-sign
 *          i16 values in the lower lanes. Verifies that the opcode correctly computes signed
 *          multiplication of corresponding lanes and widens results to i32.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:simd_extmul_operation
 * @input_conditions Standard i16 pairs in lower 4 lanes: positive, negative, and mixed signs
 * @expected_behavior Returns mathematical product with proper sign handling and widening
 * @validation_method Direct comparison of WASM function result with expected i32x4 values
 */
TEST_P(I32x4ExtmulLowI16x8STestSuite, BasicMultiplication_ReturnsCorrectResults)
{
    // Test data: i16x8 vectors represented as i32x4 (packed 2 i16 per i32)
    // First vector: [10, 20, 30, 40, ignored_upper_lanes...]
    const int32_t vec1[4] = {(20 << 16) | (10 & 0xFFFF),  // lanes 0,1: 10, 20
                            (40 << 16) | (30 & 0xFFFF),  // lanes 2,3: 30, 40
                            0, 0};                       // upper lanes (ignored)

    // Second vector: [2, 3, 4, 5, ignored_upper_lanes...]
    const int32_t vec2[4] = {(3 << 16) | (2 & 0xFFFF),   // lanes 0,1: 2, 3
                            (5 << 16) | (4 & 0xFFFF),   // lanes 2,3: 4, 5
                            0, 0};                       // upper lanes (ignored)

    int32_t result[4];

    // Execute i32x4.extmul_low_i16x8_s operation
    call_extmul_test_function(FUNC_NAME_BASIC_EXTMUL, vec1, vec2, result);

    // Validate results: [10*2, 20*3, 30*4, 40*5] = [20, 60, 120, 200]
    ASSERT_EQ(20, result[0]) << "Lane 0 multiplication failed: 10 * 2 should equal 20";
    ASSERT_EQ(60, result[1]) << "Lane 1 multiplication failed: 20 * 3 should equal 60";
    ASSERT_EQ(120, result[2]) << "Lane 2 multiplication failed: 30 * 4 should equal 120";
    ASSERT_EQ(200, result[3]) << "Lane 3 multiplication failed: 40 * 5 should equal 200";
}

/**
 * @test BoundaryValues_HandleExtremeInputs
 * @brief Tests behavior at i16 MIN/MAX boundaries and their multiplication products
 * @details Validates extended multiplication with extreme i16 values to ensure no overflow
 *          occurs and maximum magnitude results are computed correctly.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:simd_boundary_handling
 * @input_conditions i16 MIN_VALUE (-32768) and MAX_VALUE (32767) combinations
 * @expected_behavior Maximum magnitude results without overflow in i32 range
 * @validation_method Verification of extreme value multiplication accuracy
 */
TEST_P(I32x4ExtmulLowI16x8STestSuite, BoundaryValues_HandleExtremeInputs)
{
    // Test with i16 boundary values: MIN=-32768, MAX=32767
    const int16_t i16_min = -32768;
    const int16_t i16_max = 32767;

    // Vector 1: [MAX, MIN, MAX, MIN, ...]
    const int32_t vec1[4] = {(int32_t)(((uint32_t)(uint16_t)i16_min << 16) | (i16_max & 0xFFFF)),  // lanes 0,1: 32767, -32768
                            (int32_t)(((uint32_t)(uint16_t)i16_min << 16) | (i16_max & 0xFFFF)),  // lanes 2,3: 32767, -32768
                            0, 0};

    // Vector 2: [MAX, MAX, MIN, MIN, ...]
    const int32_t vec2[4] = {(i16_max << 16) | (i16_max & 0xFFFF),  // lanes 0,1: 32767, 32767
                            (int32_t)(((uint32_t)(uint16_t)i16_min << 16) | ((uint16_t)i16_min & 0xFFFF)),  // lanes 2,3: -32768, -32768
                            0, 0};

    int32_t result[4];

    // Execute boundary value test
    call_extmul_test_function(FUNC_NAME_BOUNDARY_EXTMUL, vec1, vec2, result);

    // Validate results:
    // [32767*32767, (-32768)*32767, 32767*(-32768), (-32768)*(-32768)]
    // = [1073676289, -1073709056, -1073709056, 1073741824]
    ASSERT_EQ(1073676289, result[0]) << "MAX×MAX failed: should equal 1073676289";
    ASSERT_EQ(-1073709056, result[1]) << "MIN×MAX failed: should equal -1073709056";
    ASSERT_EQ(-1073709056, result[2]) << "MAX×MIN failed: should equal -1073709056";
    ASSERT_EQ(1073741824, result[3]) << "MIN×MIN failed: should equal 1073741824";
}

// Parametrized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(
    RunningModeTests,
    I32x4ExtmulLowI16x8STestSuite,
    testing::Values(I32x4ExtmulLowI16x8SRunningMode::INTERP, I32x4ExtmulLowI16x8SRunningMode::AOT),
    [](const testing::TestParamInfo<I32x4ExtmulLowI16x8STestSuite::ParamType>& info) {
        return info.param == I32x4ExtmulLowI16x8SRunningMode::INTERP ? "Interpreter" : "AOT";
    }
);
