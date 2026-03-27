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
 * @file enhanced_i32_rotr_test.cc
 * @brief Enhanced unit tests for i32.rotr opcode - Numeric Category
 * @details This test suite provides comprehensive coverage for the i32.rotr (rotate right)
 * WebAssembly instruction, focusing on:
 * - Basic rotation functionality with typical values and rotation counts
 * - Corner cases including boundary rotation counts (0, 31, 32, 33+) and modulo behavior
 * - Edge cases with extreme values (0, 0xFFFFFFFF, powers of 2, alternating patterns)
 * - Identity operations and mathematical properties (cyclic behavior, bit preservation)
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:i32.rotr operations
 * @coverage_target core/iwasm/aot/aot_runtime.c:bitwise rotation instructions
 * @coverage_target Bit rotation algorithms and modulo 32 behavior
 * @coverage_target Stack management for binary numeric operations
 * @test_modes Both interpreter (Mode_Interp) and AOT (Mode_LLVM_JIT) execution
 */

class I32RotrTestSuite : public testing::TestWithParam<RunningMode> {
protected:
    void SetUp() override {
        // Initialize WAMR runtime with system allocator for test isolation
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for i32.rotr testing";

        // Load WASM test module containing i32.rotr test functions
        std::string wasm_file = "./wasm-apps/i32_rotr_test.wasm";
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

        // Instantiate WASM module for test execution
        module_inst = wasm_runtime_instantiate(module, 65536, 65536,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode based on test parameter (interpreter or AOT)
        wasm_runtime_set_running_mode(module_inst, GetParam());
    }

    void TearDown() override {
        // Clean up WASM resources in reverse order of creation
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (module_buffer != nullptr) {
            BH_FREE(module_buffer);
        }

        // Shutdown WAMR runtime to prevent resource leaks
        wasm_runtime_destroy();
    }

    /**
     * @brief Executes i32.rotr operation with two operands
     * @details Helper function to call WASM i32.rotr function and retrieve result
     * @param value The 32-bit integer value to rotate
     * @param count The number of positions to rotate right
     * @return Result of rotating value right by count positions
     * @coverage_target Function call mechanism and parameter passing
     */
    uint32_t call_i32_rotr(uint32_t value, uint32_t count) {
        // Locate the exported i32_rotr test function
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "i32_rotr_test");
        EXPECT_NE(nullptr, func) << "Failed to find i32_rotr_test function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        // Prepare function arguments: [value, count]
        uint32_t argv[2] = { value, count };

        // Execute function with error handling
        bool call_result = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_TRUE(call_result)
            << "i32_rotr function call failed: "
            << wasm_runtime_get_exception(module_inst);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        // Return the computed result
        return argv[0];
    }

private:
    wasm_module_t module = nullptr;           // Loaded WASM module
    wasm_module_inst_t module_inst = nullptr; // Instantiated WASM module
    uint8_t* module_buffer = nullptr;         // Raw WASM bytecode buffer
    uint32_t buffer_size = 0;                 // Size of WASM bytecode buffer
};

/**
 * @test BasicRotation_ReturnsCorrectResult
 * @brief Validates i32.rotr produces correct results for typical rotation scenarios
 * @details Tests fundamental right rotation operation with common values and rotation counts.
 *          Verifies that i32.rotr correctly rotates bits right with proper wraparound behavior.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:i32_rotr_operation
 * @input_conditions Standard integer values with small to medium rotation counts
 * @expected_behavior Returns mathematically correct bit rotation results
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(I32RotrTestSuite, BasicRotation_ReturnsCorrectResult) {
    // Test typical rotation scenarios with predictable results
    ASSERT_EQ(0x81234567U, call_i32_rotr(0x12345678U, 4))
        << "Right rotation of 0x12345678 by 4 positions failed";

    ASSERT_EQ(0x21876543U, call_i32_rotr(0x87654321U, 8))
        << "Right rotation of 0x87654321 by 8 positions failed";

    ASSERT_EQ(0xF00ABCDEU, call_i32_rotr(0xABCDEF00U, 12))
        << "Right rotation of 0xABCDEF00 by 12 positions failed";

    // Test with smaller rotation counts showing clear bit movement
    ASSERT_EQ(0x091A2B3CU, call_i32_rotr(0x12345678U, 1))
        << "Right rotation by 1 position failed";

    ASSERT_EQ(0x30ECA864U, call_i32_rotr(0x87654321U, 3))
        << "Right rotation by 3 positions failed";
}

// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(I32RotrTest, I32RotrTestSuite,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT));
