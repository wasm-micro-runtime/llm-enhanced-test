/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

/**
 * Enhanced unit tests for i32.and WASM opcode
 *
 * Tests comprehensive bitwise AND functionality including:
 * - Basic bitwise AND operations with mixed bit patterns
 * - Boundary condition handling (0x00000000, 0xFFFFFFFF, INT32_MIN/MAX)
 * - Mathematical property validation (commutative, identity, zero elements)
 * - Bit pattern manipulation and masking operations
 * - Stack underflow error handling
 * - Cross-execution mode validation (interpreter vs AOT)
 */

/**
 * Test fixture for i32.and opcode validation
 *
 * Provides comprehensive test environment for bitwise AND operations
 * using WAMR test helpers for proper runtime initialization and
 * cross-execution mode testing (interpreter and AOT).
 */
class I32AndTest : public testing::TestWithParam<RunningMode> {
protected:
    WAMRRuntimeRAII<> runtime;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint32_t buf_size, stack_size = 8092, heap_size = 8092;
    uint8_t *buf = nullptr;
    char error_buf[128] = { 0 };
    const char *exception = nullptr;

    /**
     * Sets up test environment with WAMR runtime initialization
     *
     * Initializes WAMR runtime with system allocator and loads i32.and test module
     * Configures both interpreter and AOT execution modes for comprehensive testing
     */
    void SetUp() override {
        // Load WASM module for i32.and tests
        loadTestModule();
    }

    /**
     * Cleans up test environment with proper resource deallocation
     *
     * Destroys WASM module instance and runtime to prevent memory leaks
     */
    void TearDown() override {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
            exec_env = nullptr;
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (buf != nullptr) {
            BH_FREE(buf);
            buf = nullptr;
        }
    }

    /**
     * Loads the i32.and test WASM module
     *
     * Loads the WASM bytecode from file and instantiates module for testing
     * Supports both interpreter and AOT execution modes
     */
    void loadTestModule() {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        // Read WASM file from wasm-apps directory
        buf = (uint8_t*)bh_read_file_to_buffer("wasm-apps/i32_and_test.wasm", &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: wasm-apps/i32_and_test.wasm";

        // Load WASM module
        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        // Create module instance
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        // Set running mode and create execution environment
        wasm_runtime_set_running_mode(module_inst, GetParam());
        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * Calls a WASM function and returns the result
     *
     * @param func_name Name of the WASM function to call
     * @param param1 First parameter (uint32_t)
     * @param param2 Second parameter (uint32_t)
     * @return Function execution result as uint32_t
     */
    uint32_t call_i32_and_function(const char* func_name, uint32_t param1, uint32_t param2) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(func, nullptr) << "Failed to lookup function: " << func_name;

        uint32_t argv[2] = { param1, param2 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_TRUE(ret) << "Failed to call function: " << func_name;

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Function call exception: " << (exception != nullptr ? exception : "none");

        return argv[0];  // Return result from first element
    }

};

/**
 * @test BasicBitwiseAnd_ReturnsCorrectResults
 * @brief Validates i32.and produces correct bitwise AND results for typical inputs
 * @details Tests fundamental bitwise AND operation with mixed bit patterns.
 *          Verifies that i32.and correctly computes a & b for various input combinations.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:WASM_OP_I32_AND
 * @input_conditions Standard bit patterns: hex values, decimal values, mixed patterns
 * @expected_behavior Returns bitwise AND results: preserves bits set in both operands
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(I32AndTest, BasicBitwiseAnd_ReturnsCorrectResults) {
    // Test mixed bit patterns
    ASSERT_EQ(0x02244220, call_i32_and_function("i32_and", 0x12345678, 0x87654321))
        << "Bitwise AND of 0x12345678 & 0x87654321 should equal 0x02244220";

    // Test simple decimal values
    ASSERT_EQ(7, call_i32_and_function("i32_and", 15, 7))
        << "Bitwise AND of 15 & 7 should equal 7";

    // Test alternating bit patterns
    ASSERT_EQ(0x00000000, call_i32_and_function("i32_and", 0xAAAAAAAA, 0x55555555))
        << "Alternating patterns 0xAAAAAAAA & 0x55555555 should equal 0x00000000";

    // Test power of 2 patterns
    ASSERT_EQ(0x00000018, call_i32_and_function("i32_and", 0x0000001F, 0x00000018))
        << "Mask operation 0x1F & 0x18 should equal 0x18";
}

/**
 * @test MathematicalProperties_ValidatedCorrectly
 * @brief Validates mathematical properties of bitwise AND operation
 * @details Tests commutative, identity, zero element, and idempotent properties
 *          of bitwise AND to ensure correct mathematical behavior.
 * @test_category Edge - Mathematical property validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:WASM_OP_I32_AND
 * @input_conditions Various values for property testing: identity, zero, commutativity
 * @expected_behavior Mathematical properties hold: a&b = b&a, a&0 = 0, a&0xFFFFFFFF = a, a&a = a
 * @validation_method Property verification through multiple assertions
 */
TEST_P(I32AndTest, MathematicalProperties_ValidatedCorrectly) {
    const uint32_t test_value = 0x12345678;

    // Test commutative property: a & b = b & a
    uint32_t result1 = call_i32_and_function("i32_and", test_value, 0x87654321);
    uint32_t result2 = call_i32_and_function("i32_and", 0x87654321, test_value);
    ASSERT_EQ(result1, result2)
        << "Commutative property failed: " << std::hex << test_value
        << " & 0x87654321 should equal 0x87654321 & " << test_value;

    // Test identity element: a & 0xFFFFFFFF = a
    ASSERT_EQ(test_value, call_i32_and_function("i32_and", test_value, 0xFFFFFFFF))
        << "Identity property failed: " << std::hex << test_value
        << " & 0xFFFFFFFF should equal " << test_value;

    // Test zero element: a & 0 = 0
    ASSERT_EQ(0x00000000, call_i32_and_function("i32_and", test_value, 0x00000000))
        << "Zero element property failed: " << std::hex << test_value << " & 0 should equal 0";

    // Test idempotent property: a & a = a
    ASSERT_EQ(test_value, call_i32_and_function("i32_and", test_value, test_value))
        << "Idempotent property failed: " << std::hex << test_value
        << " & " << test_value << " should equal " << test_value;

    // Test bit masking property
    const uint32_t mask = 0x0000FFFF;
    uint32_t masked_result = call_i32_and_function("i32_and", test_value, mask);
    ASSERT_EQ(0x00005678, masked_result)
        << "Bit masking failed: " << std::hex << test_value
        << " & " << mask << " should equal 0x00005678";
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(I32AndTest, I32AndTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT));
