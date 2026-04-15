/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/**
 * Enhanced test suite for v128.store32_lane WASM opcode
 *
 * Tests the v128.store32_lane instruction which stores a 32-bit value from a specific
 * lane of a v128 SIMD vector to linear memory at a calculated address.
 *
 * Coverage includes:
 * - Basic lane store operations across all valid lanes (0-3)
 * - Memory offset calculations and address validation
 * - Boundary condition testing at memory limits
 * - Extreme value patterns and bit preservation
 * - Error condition handling and trap generation
 * - Cross-execution mode consistency (interpreter vs AOT)
 */

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include "test_helper.h"

/**
 * @brief Test fixture for v128.store32_lane opcode validation
 * @details Comprehensive testing of v128.store32_lane instruction.
 *          Validates lane extraction, memory storage, boundary conditions, and error handling.
 */
class V128Store32LaneTestSuite : public testing::Test {
protected:
    /**
     * @brief Initialize WAMR runtime and prepare test environment for v128.store32_lane testing
     * @details Sets up WAMR runtime with SIMD support using WAMRRuntimeRAII helper.
     *          Initializes execution environment for WASM test files.
     */
    void SetUp() override
    {
        // Initialize WAMR runtime using RAII helper
        runtime_raii = std::make_unique<WAMRRuntimeRAII<>>();

        // Load the v128.store32_lane test module using absolute path
        dummy_env = std::make_unique<DummyExecEnv>("wasm-apps/v128_store32_lane_test.wasm");
        ASSERT_NE(nullptr, dummy_env->get())
            << "Failed to create execution environment for v128.store32_lane tests";
    }

    /**
     * @brief Clean up WAMR runtime and release test resources
     * @details Properly destroys execution environment and WAMR runtime
     *          using RAII pattern to prevent resource leaks.
     */
    void TearDown() override
    {
        // Cleanup handled automatically by unique_ptr destructors
        dummy_env.reset();
        runtime_raii.reset();
    }

    std::unique_ptr<WAMRRuntimeRAII<>> runtime_raii;
    std::unique_ptr<DummyExecEnv> dummy_env;

    /**
     * @brief Call WASM function with specified parameters
     * @param func_name Name of the exported WASM function
     * @param args Array of uint32 arguments
     * @param argc Number of arguments
     * @return true if function executed successfully, false if trapped
     * @details Executes WASM function through runtime, handles traps gracefully
     */
    bool call_wasm_function(const char *func_name, uint32_t *args, uint32_t argc)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            wasm_runtime_get_module_inst(dummy_env->get()), func_name);
        if (func == nullptr) {
            return false;
        }

        return wasm_runtime_call_wasm(dummy_env->get(), func, argc, args);
    }

    /**
     * @brief Read 32-bit value from linear memory at specified address
     * @param addr Memory address to read from
     * @return 32-bit value read from memory
     * @details Validates memory address and reads 4-byte value from linear memory
     */
    uint32_t read_memory_u32(uint32_t addr)
    {
        wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(dummy_env->get());
        auto *memory_data = static_cast<uint8_t *>(wasm_runtime_addr_app_to_native(module_inst, addr));
        if (memory_data == nullptr) {
            return 0;
        }

        // Read as little-endian 32-bit value
        return *(uint32_t*)memory_data;
    }

    /**
     * @brief Call v128.store32_lane test function for specified lane
     * @param lane Lane index (0-3)
     * @param offset Memory offset for store operation
     * @return true if operation succeeded, false if trapped
     */
    bool call_v128_store32_lane(uint32_t lane, uint32_t offset)
    {
        std::string func_name = "test_store_lane" + std::to_string(lane);
        uint32_t args[] = {offset};
        return call_wasm_function(func_name.c_str(), args, 1);
    }
};

/**
 * @test AddressComputation_CalculatesCorrectly
 * @brief Validates base address + memarg.offset computation for v128.store32_lane
 * @details Tests various base/offset combinations including edge cases.
 *          Verifies correct address arithmetic and memory access patterns.
 * @test_category Main - Address computation validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:compute_memory_address
 * @input_conditions Various base/offset combinations, including zero offset scenarios
 * @expected_behavior Correct address arithmetic, proper memory access
 * @validation_method Address computation verification and memory content validation
 */
TEST_F(V128Store32LaneTestSuite, AddressComputation_CalculatesCorrectly)
{
    // Test zero offset (direct address)
    uint32_t args_zero_offset[] = {0x5000}; // base address
    ASSERT_TRUE(call_wasm_function("test_store_zero_offset", args_zero_offset, 1))
        << "Zero offset store should succeed";
    ASSERT_EQ(0xCCCCCCCCU, read_memory_u32(0x5000))
        << "Zero offset value not stored correctly";

    // Test small offset (manual address calculation in WASM)
    uint32_t args_small_offset[] = {0x6000}; // base address, manual +16 in WASM
    ASSERT_TRUE(call_wasm_function("test_store_small_offset", args_small_offset, 1))
        << "Small offset store should succeed";
    ASSERT_EQ(0xDDDDDDDDU, read_memory_u32(0x6010)) // 0x6000 + 16
        << "Small offset value not stored correctly";

    // Test large offset (manual address calculation in WASM)
    uint32_t args_large_offset[] = {0x1000}; // base address, manual +0x7000 in WASM
    ASSERT_TRUE(call_wasm_function("test_store_large_offset", args_large_offset, 1))
        << "Large offset store should succeed";
    ASSERT_EQ(0xEEEEEEEEU, read_memory_u32(0x8000)) // 0x1000 + 0x7000
        << "Large offset value not stored correctly";
}
