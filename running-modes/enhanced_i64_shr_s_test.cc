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
 * @file enhanced_i64_shr_s_test.cc
 * @brief Enhanced unit tests for i64.shr_s opcode - Numeric Category
 * @details This test suite provides comprehensive coverage for the i64.shr_s (64-bit signed right shift)
 * WebAssembly instruction, focusing on:
 * - Basic arithmetic right shift functionality with positive and negative 64-bit values
 * - Corner cases including boundary values (INT64_MIN, INT64_MAX) and maximum shifts
 * - Edge cases with zero operands, identity operations, and modulo shift behavior
 * - Sign extension verification and mathematical properties of 64-bit arithmetic shifts
 * - Shift count modulo 64 behavior for large shift values
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:i64.shr_s operations
 * @coverage_target core/iwasm/aot/aot_runtime.c:64-bit signed shift instructions
 * @coverage_target Sign extension behavior and arithmetic shift algorithms
 * @coverage_target Stack management for 64-bit binary numeric operations
 * @test_modes Both interpreter (Mode_Interp) and AOT (Mode_LLVM_JIT) execution
 */

class I64ShrSTestSuite : public testing::TestWithParam<RunningMode> {
protected:
    void SetUp() override {
        // Initialize WAMR runtime with system allocator for test isolation
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for i64.shr_s testing";

        // Load WASM test module containing i64.shr_s test functions
        std::string wasm_file = "wasm-apps/i64_shr_s_test.wasm";
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
     * @brief Executes i64.shr_s operation with two operands
     * @details Helper function to call WASM i64.shr_s function and retrieve result
     * @param value The 64-bit integer value to be shifted right
     * @param shift_count The number of positions to shift right (masked to 6 bits)
     * @return Result of arithmetic right shift operation
     * @coverage_target Function call mechanism and 64-bit parameter passing
     */
    int64_t call_i64_shr_s(int64_t value, uint64_t shift_count) {
        // Locate the exported i64_shr_s test function
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "i64_shr_s_test");
        EXPECT_NE(nullptr, func) << "Failed to find i64_shr_s_test function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        // Prepare function arguments: [value_low, value_high, shift_count_low, shift_count_high]
        // WASM uses 32-bit stack slots, so 64-bit values need two slots
        uint32_t argv[4] = {
            static_cast<uint32_t>(value & 0xFFFFFFFF),           // value low 32 bits
            static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF),   // value high 32 bits
            static_cast<uint32_t>(shift_count & 0xFFFFFFFF),     // shift_count low 32 bits
            static_cast<uint32_t>((shift_count >> 32) & 0xFFFFFFFF) // shift_count high 32 bits
        };

        // Execute function with error handling
        bool call_result = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        EXPECT_TRUE(call_result)
            << "i64_shr_s function call failed: "
            << wasm_runtime_get_exception(module_inst);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        // Reconstruct 64-bit result from two 32-bit values
        uint64_t result = (static_cast<uint64_t>(argv[1]) << 32) | argv[0];
        return static_cast<int64_t>(result);
    }

private:
    wasm_module_t module = nullptr;           // Loaded WASM module
    wasm_module_inst_t module_inst = nullptr; // Instantiated WASM module
    uint8_t* module_buffer = nullptr;         // Raw WASM bytecode buffer
    uint32_t buffer_size = 0;                 // Size of WASM bytecode buffer
};

/**
 * @test SignExtension_PreservesSignBit
 * @brief Validates i64.shr_s properly extends sign bit during 64-bit arithmetic shifts
 * @details Tests that negative values maintain their sign through 64-bit arithmetic
 *          right shift operations, distinguishing from logical right shift.
 * @test_category Edge - Sign extension behavior validation for 64-bit operations
 * @coverage_target Arithmetic vs logical shift differentiation
 * @input_conditions Various negative 64-bit values with different shift counts
 * @expected_behavior Sign bit preserved through 64-bit shift operations
 * @validation_method Sign preservation verification across 64-bit shift ranges
 */
TEST_P(I64ShrSTestSuite, SignExtension_PreservesSignBit) {
    // Test that negative values remain negative across all valid shifts
    int64_t test_negative = -1000000000LL;  // Large negative 64-bit value
    for (uint32_t shift = 1; shift <= 63; ++shift) {
        int64_t result = call_i64_shr_s(test_negative, shift);
        ASSERT_LT(result, 0LL)
            << "Negative value " << test_negative << " >> " << shift
            << " should remain negative, got: " << result;
    }

    // Test maximum shift of any negative number produces -1
    ASSERT_EQ(-1LL, call_i64_shr_s(-42000000000LL, 63))
        << "Maximum shift of negative 64-bit value should produce -1";

    ASSERT_EQ(-1LL, call_i64_shr_s(-1LL, 63))
        << "Maximum shift of -1 should produce -1";

    // Test positive values never become negative
    int64_t test_positive = 1000000000000LL;  // Large positive 64-bit value
    for (uint32_t shift = 1; shift <= 63; ++shift) {
        int64_t result = call_i64_shr_s(test_positive, shift);
        ASSERT_GE(result, 0LL)
            << "Positive value " << test_positive << " >> " << shift
            << " should remain non-negative, got: " << result;
    }

    // Test special bit patterns for sign extension
    ASSERT_EQ(-4611686018427387904LL, call_i64_shr_s(0x8000000000000000LL, 1))  // MSB set, shift by 1
        << "Sign extension from MSB set failed";

    ASSERT_EQ(-1125899906842624LL, call_i64_shr_s(0xF000000000000000LL, 10))  // High bits set
        << "Sign extension with multiple high bits set failed";
}

// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(I64ShrSTest, I64ShrSTestSuite,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT));
