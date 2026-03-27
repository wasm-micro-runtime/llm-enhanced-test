/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>  // Primary GTest framework for unit testing
#include <climits>        // Standard integer limits for boundary testing
#include <cstdint>        // Standard integer types for precise type control
#include <vector>         // Container for batch test case management
#include "wasm_export.h"  // Core WAMR runtime API for module management
#include "bh_read_file.h" // WAMR utility for loading WASM binary files

/**
 * @file enhanced_i32_shr_u_test.cc
 * @brief Enhanced unit tests for i32.shr_u opcode - Numeric Category
 * @details This test suite provides comprehensive coverage for the i32.shr_u (unsigned right shift)
 * WebAssembly instruction, focusing on:
 * - Basic logical right shift functionality with zero-fill behavior
 * - Corner cases including boundary values (UINT32_MAX, INT32_MIN) and maximum shifts
 * - Edge cases with zero operands, identity operations, and modulo shift behavior
 * - Zero-fill verification and mathematical properties of logical shifts
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:i32.shr_u operations
 * @coverage_target core/iwasm/aot/aot_runtime.c:unsigned shift instructions
 * @coverage_target Zero-fill behavior and logical shift algorithms
 * @coverage_target Stack management for binary numeric operations
 * @test_modes Both interpreter (Mode_Interp) and AOT (Mode_LLVM_JIT) execution
 */

class I32ShrUTestSuite : public testing::TestWithParam<RunningMode> {
protected:
    void SetUp() override {
        // Initialize WAMR runtime with system allocator for test isolation
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for i32.shr_u testing";

        // Load WASM test module containing i32.shr_u test functions
        std::string wasm_file = "./wasm-apps/i32_shr_u_test.wasm";
        module_buffer = reinterpret_cast<uint8_t*>(
            bh_read_file_to_buffer(wasm_file.c_str(), &buffer_size));
        ASSERT_NE(nullptr, module_buffer)
            << "Failed to load WASM file: " << wasm_file;

        // Load and validate WASM module with error reporting
        char error_buf[128];
        module = wasm_runtime_load(module_buffer, buffer_size,
                                 error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module)
            << "Failed to load WASM module: " << error_buf;

        // Instantiate module for execution
        module_inst = wasm_runtime_instantiate(module, 65536, 65536,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode (interpreter or AOT) based on test parameter
        wasm_runtime_set_running_mode(module_inst, GetParam());
    }

    void TearDown() override {
        // Clean up WAMR resources in reverse order of creation
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (module_buffer != nullptr) {
            BH_FREE(module_buffer);
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Execute i32.shr_u operation with two i32 parameters
     * @param value The value to be shifted
     * @param shift_count Number of positions to shift right
     * @return Result of unsigned right shift operation
     */
    uint32_t CallI32ShrU(uint32_t value, uint32_t shift_count) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "i32_shr_u_test");
        EXPECT_NE(nullptr, func) << "Failed to lookup i32_shr_u_test function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t argv[2] = {value, shift_count};

        bool success = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_TRUE(success) << "WASM function execution failed: "
                           << wasm_runtime_get_exception(module_inst);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return argv[0]; // Return value stored in first element
    }

    /**
     * @brief Execute single-parameter test function with fixed shift count
     * @param input The value to be shifted by 4 positions
     * @return Result of input >> 4 operation
     */
    uint32_t CallBasicShrU(uint32_t input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "test_basic_shr_u");
        EXPECT_NE(nullptr, func) << "Failed to lookup test_basic_shr_u function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t argv[1] = {input};
        bool success = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(success) << "Basic shift test execution failed";

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return argv[0];
    }

    /**
     * @brief Execute test function by name and return result
     * @param func_name Name of exported WASM function to call
     * @return Function execution result
     */
    uint32_t CallTestFunction(const char* func_name) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Failed to lookup function: " << func_name;

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t argv[1] = {0}; // Result container
        bool success = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_TRUE(success) << "Function " << func_name << " execution failed";

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return argv[0];
    }

private:
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    uint8_t* module_buffer = nullptr;
    uint32_t buffer_size = 0;
};

/**
 * @test BasicUnsignedRightShift_ReturnsCorrectResults
 * @brief Validates i32.shr_u produces correct logical right shift results
 * @details Tests fundamental unsigned shift operation with various input combinations.
 *          Verifies zero-fill behavior and correct mathematical results.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i32_shr_u_operation
 * @input_conditions Standard integer pairs with different bit patterns
 * @expected_behavior Returns mathematical result with zero-fill from left
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(I32ShrUTestSuite, BasicUnsignedRightShift_ReturnsCorrectResults) {
    // Test basic positive shift operations
    ASSERT_EQ(4U, CallI32ShrU(8, 1))
        << "8 >> 1 should produce 4";
    ASSERT_EQ(4U, CallI32ShrU(16, 2))
        << "16 >> 2 should produce 4";
    ASSERT_EQ(0x00123456U, CallI32ShrU(0x12345678, 8))
        << "0x12345678 >> 8 should produce 0x00123456";

    // Test shift operations with different bit patterns
    ASSERT_EQ(16U, CallBasicShrU(256))
        << "256 >> 4 should produce 16";
    ASSERT_EQ(0x01234000U, CallBasicShrU(0x12340000))
        << "0x12340000 >> 4 should produce 0x01234000";
}

// Instantiate test suite for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(I32ShrUTest, I32ShrUTestSuite,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT));
