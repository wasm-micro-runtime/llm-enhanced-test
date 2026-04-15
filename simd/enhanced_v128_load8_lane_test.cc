/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include "test_helper.h"

/**
 * @class V128Load8LaneTestSuite
 * @brief Test fixture for v128.load8_lane opcode validation
 * @details Validates SIMD memory load operations with lane insertion for 8-bit values
 *          Tests both interpreter and AOT execution modes with comprehensive validation
 */
class V128Load8LaneTestSuite : public testing::Test
{
protected:
    /**
     * @brief Initialize WAMR runtime and prepare test environment for v128.load8_lane testing
     * @details Sets up WAMR runtime with SIMD support using WAMRRuntimeRAII helper.
     *          Initializes execution environment for WASM test files.
     */
    void SetUp() override
    {
        // Initialize WAMR runtime using RAII helper
        runtime_raii = std::make_unique<WAMRRuntimeRAII<>>();

        // Load the v128.load8_lane test module using absolute path
        dummy_env = std::make_unique<DummyExecEnv>("wasm-apps/v128_load8_lane_test.wasm");
        ASSERT_NE(nullptr, dummy_env->get())
            << "Failed to create execution environment for v128.load8_lane tests";
    }

    /**
     * @brief Clean up WAMR runtime and release test resources
     * @details Properly destroys execution environment and WAMR runtime
     *          using RAII pattern to prevent resource leaks.
     */
    void TearDown() override
    {
        // Cleanup handled by RAII destructors
        dummy_env.reset();
        runtime_raii.reset();
    }

    std::unique_ptr<WAMRRuntimeRAII<>> runtime_raii;
    std::unique_ptr<DummyExecEnv> dummy_env;

    /**
     * @brief Helper function to call v128.load8_lane WASM function
     * @param func_name Name of the exported WASM function
     * @param address Memory address for load operation
     * @param result_hi Reference to store high 64 bits of result v128 vector
     * @param result_lo Reference to store low 64 bits of result v128 vector
     * @return bool True if operation succeeded, false on error
     */
    bool call_v128_load8_lane(const char *func_name, uint32_t address,
                             uint64_t& result_hi, uint64_t& result_lo)
    {
        // Prepare arguments: one i32 address input + space for v128 result (4 uint32_t)
        uint32_t argv[5];  // 1 for input i32 + 4 for v128 result
        argv[0] = address;

        // Call WASM function with one i32 input, returns v128
        bool call_success = dummy_env->execute(func_name, 1, argv);
        if (!call_success) {
            return false;
        }

        // Extract v128 result from argv (returned as 4 uint32_t values)
        // Following the pattern from existing v128 tests
        result_lo = (static_cast<uint64_t>(argv[1]) << 32) | argv[0];
        result_hi = (static_cast<uint64_t>(argv[3]) << 32) | argv[2];

        return true;
    }

    /**
     * @brief Helper function to extract byte from v128 at specified lane
     * @param v128_hi High 64 bits of v128 vector
     * @param v128_lo Low 64 bits of v128 vector
     * @param lane_index Lane index (0-15)
     * @return The byte value at the specified lane
     */
    uint8_t extract_byte_from_v128(uint64_t v128_hi, uint64_t v128_lo, uint32_t lane_index)
    {
        // Convert v128 to byte array for easy access
        uint8_t bytes[16];
        memcpy(&bytes[0], &v128_lo, 8);  // First 8 bytes (lanes 0-7)
        memcpy(&bytes[8], &v128_hi, 8);  // Next 8 bytes (lanes 8-15)

        return bytes[lane_index];
    }

    /**
     * @brief Helper function to validate v128 lane values
     * @param v128_hi High 64 bits of result v128 vector
     * @param v128_lo Low 64 bits of result v128 vector
     * @param target_lane Lane that should contain the loaded value
     * @param expected_value Expected byte value in target lane
     * @param other_lanes_zero Whether other lanes should be zero (default true)
     */
    void validate_v128_lanes(uint64_t v128_hi, uint64_t v128_lo, uint32_t target_lane,
                            uint8_t expected_value, bool other_lanes_zero = true)
    {
        // Validate target lane contains expected value
        uint8_t actual_value = extract_byte_from_v128(v128_hi, v128_lo, target_lane);
        ASSERT_EQ(actual_value, expected_value)
            << "Lane " << target_lane << " should contain " << (int)expected_value
            << " but got " << (int)actual_value;

        // Optionally validate other lanes are zero
        if (other_lanes_zero) {
            for (uint32_t lane = 0; lane < 16; lane++) {
                if (lane != target_lane) {
                    uint8_t lane_value = extract_byte_from_v128(v128_hi, v128_lo, lane);
                    ASSERT_EQ(lane_value, 0)
                        << "Lane " << lane << " should be zero but got " << (int)lane_value;
                }
            }
        }
    }
};

/**
 * @test AllLanesCoverage_ValidatesCompleteLaneAccess
 * @brief Test loading into all 16 possible lanes (0-15) with different byte values
 * @details Validates that v128.load8_lane can successfully access all 16 byte lanes
 *          within a v128 vector with sequential byte patterns.
 * @test_category Main - Comprehensive lane access validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:v128_load8_lane_lane_validation
 * @input_conditions Sequential byte values (0x00-0x0F), corresponding lane indices
 * @expected_behavior Each lane contains its corresponding byte value
 * @validation_method Loop-based validation of all 16 lanes with descriptive messages
 */
TEST_F(V128Load8LaneTestSuite, AllLanesCoverage_ValidatesCompleteLaneAccess)
{
    uint64_t result_hi, result_lo;

    // Test comprehensive lane loading using the test_all_lanes function
    // This function loads sequential bytes (0x00-0x0F) into lanes 0-15
    ASSERT_TRUE(call_v128_load8_lane("test_all_lanes", 0x00, result_hi, result_lo))
        << "Failed to call v128.load8_lane comprehensive test function";

    // Verify each lane contains its corresponding byte value
    for (uint32_t lane = 0; lane < 16; lane++) {
        auto expected_byte = (uint8_t)lane;
        uint8_t actual_byte = extract_byte_from_v128(result_hi, result_lo, lane);

        ASSERT_EQ(actual_byte, expected_byte)
            << "Lane " << lane << " should contain byte value " << (int)expected_byte
            << " but got " << (int)actual_byte;
    }
}
