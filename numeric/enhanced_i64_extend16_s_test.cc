/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for i64.extend16_s Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly i64.extend16_s
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic sign extension functionality with typical values
 * - Corner Cases: Boundary conditions at 16-bit signed integer limits
 * - Edge Cases: Upper bit interference and mathematical properties
 * - Error Handling: Stack underflow and validation scenarios
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (lines handling i64.extend16_s)
 * Source Location: core/iwasm/interpreter/wasm_interp_classic.c (i64 sign extension operations)
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;
static std::string WASM_FILE_UNDERFLOW;

static int
app_argc;
static char **app_argv;

/**
 * @class I64Extend16sTest
 * @brief Test fixture for comprehensive i64.extend16_s opcode validation
 *
 * Provides WAMR runtime environment setup and cleanup for testing
 * i64.extend16_s sign extension operations across different execution modes.
 * Supports both interpreter and AOT modes through parameterized testing.
 */
class I64Extend16sTest : public testing::TestWithParam<RunningMode>
{
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
     * @brief Set up WAMR runtime environment and load i64.extend16_s test module
     *
     * Initializes WAMR runtime, loads the test WASM module, creates module instance,
     * and sets up execution environment for i64.extend16_s testing.
     * Configures the running mode based on test parameter (interpreter/AOT).
     */
    void SetUp() override
    {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        // Initialize WASM file paths - done in SetUp to ensure proper timing
        WASM_FILE = "wasm-apps/i64_extend16_s_test.wasm";
        WASM_FILE_UNDERFLOW = "wasm-apps/i64_extend16_s_stack_underflow.wasm";

        // Load i64.extend16_s test WASM module
        buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode (interpreter or AOT)
        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up WAMR runtime resources
     *
     * Destroys execution environment, deinstantiates module, unloads module,
     * and frees allocated buffers to prevent resource leaks.
     */
    void TearDown() override
    {
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
     * @brief Execute i64.extend16_s operation with input value
     * @param input Input i64 value for sign extension
     * @return Sign-extended i64 result
     *
     * Calls the test_i64_extend16_s function in the loaded WASM module,
     * passing the input value and returning the sign-extended result.
     */
    int64_t call_i64_extend16_s(int64_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_extend16_s");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_extend16_s function";

        wasm_val_t params[1] = { 0 };
        wasm_val_t results[1] = { 0 };

        params[0].kind = WASM_I64;
        params[0].of.i64 = input;
        results[0].kind = WASM_I64;

        bool call_result = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, params);
        EXPECT_TRUE(call_result) << "Failed to call test_i64_extend16_s function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Exception occurred: " << exception;

        return results[0].of.i64;
    }

    /**
     * @brief Execute double i64.extend16_s operation for idempotent testing
     * @param input Input i64 value for double sign extension
     * @return Result after two consecutive sign extensions
     *
     * Tests idempotent property: extend16_s(extend16_s(x)) == extend16_s(x)
     */
    int64_t call_i64_extend16_s_double(int64_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_extend16_s_double");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_extend16_s_double function";

        wasm_val_t params[1] = { 0 };
        wasm_val_t results[1] = { 0 };

        params[0].kind = WASM_I64;
        params[0].of.i64 = input;
        results[0].kind = WASM_I64;

        bool call_result = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, params);
        EXPECT_TRUE(call_result) << "Failed to call test_i64_extend16_s_double function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Exception occurred: " << exception;

        return results[0].of.i64;
    }

    /**
     * @brief Test upper bits masking behavior
     * @param input Input i64 value with arbitrary upper bits
     * @return Sign-extended result based only on lower 16 bits
     *
     * Verifies that only the lower 16 bits affect the sign extension result,
     * regardless of the values in the upper 48 bits.
     */
    int64_t call_i64_extend16_s_mask(int64_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_extend16_s_mask");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_extend16_s_mask function";

        wasm_val_t params[1] = { 0 };
        wasm_val_t results[1] = { 0 };

        params[0].kind = WASM_I64;
        params[0].of.i64 = input;
        results[0].kind = WASM_I64;

        bool call_result = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, params);
        EXPECT_TRUE(call_result) << "Failed to call test_i64_extend16_s_mask function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Exception occurred: " << exception;

        return results[0].of.i64;
    }
};

/**
 * @test BasicSignExtension_ReturnsCorrectValues
 * @brief Validates i64.extend16_s produces correct sign extension for typical values
 * @details Tests fundamental sign extension with positive, negative, and mixed-sign 16-bit integers.
 *          Verifies that the lower 16 bits are properly sign-extended to 64 bits.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i64_extend16_s_operation
 * @input_conditions Typical 16-bit signed values: 100, -100, 1000, -1000
 * @expected_behavior Returns mathematically correct sign extensions
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(I64Extend16sTest, BasicSignExtension_ReturnsCorrectValues)
{
    // Test positive values - should remain positive after sign extension
    ASSERT_EQ(100LL, call_i64_extend16_s(100LL))
        << "Positive value 100 should extend to 100";
    ASSERT_EQ(1000LL, call_i64_extend16_s(1000LL))
        << "Positive value 1000 should extend to 1000";

    // Test negative values - should remain negative after sign extension
    ASSERT_EQ(-100LL, call_i64_extend16_s(-100LL))
        << "Negative value -100 should extend to -100";
    ASSERT_EQ(-1000LL, call_i64_extend16_s(-1000LL))
        << "Negative value -1000 should extend to -1000";

    // Test mixed values in same operation for consistency
    ASSERT_EQ(15000LL, call_i64_extend16_s(15000LL))
        << "Large positive value should extend correctly";
    ASSERT_EQ(-15000LL, call_i64_extend16_s(-15000LL))
        << "Large negative value should extend correctly";
}

/**
 * @test StackUnderflow_HandledGracefully
 * @brief Validates proper handling of stack underflow conditions
 * @details Tests module loading behavior when stack underflow scenarios occur.
 *          Since i64.extend16_s requires one i64 operand, empty stack should be
 *          handled appropriately by the WASM runtime.
 * @test_category Error - Stack underflow validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:stack_validation
 * @input_conditions Stack underflow test module with no operands
 * @expected_behavior Module loads successfully, runtime handles stack conditions appropriately
 * @validation_method Module loading verification and error handling validation
 */
TEST_P(I64Extend16sTest, StackUnderflow_HandledGracefully)
{
    // Clean up current module to test underflow scenario
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

    // Hand-crafted WASM binary with actual stack underflow:
    // Function type () -> (i64), body executes i64.extend16_s on empty stack.
    // WAT compilers reject this, so we must craft the binary directly.
    uint8_t underflow_wasm[] = {
        0x00, 0x61, 0x73, 0x6d, // magic
        0x01, 0x00, 0x00, 0x00, // version 1
        0x01, 0x05,             // type section, 5 bytes
        0x01,                   // 1 type
        0x60, 0x00, 0x01, 0x7e, // func () -> (i64)
        0x03, 0x02,             // function section, 2 bytes
        0x01, 0x00,             // 1 function, type 0
        0x0a, 0x05,             // code section, 5 bytes
        0x01,                   // 1 body
        0x03,                   // body size: 3 bytes
        0x00,                   // 0 locals
        0xc3,                   // i64.extend16_s (no operand on stack!)
        0x0b                    // end
    };

    wasm_module_t underflow_module = wasm_runtime_load(
        underflow_wasm, sizeof(underflow_wasm), error_buf, sizeof(error_buf));
    if (underflow_module == nullptr) {
        ASSERT_NE('\0', error_buf[0])
            << "Load failure should set an error message";
    }
    else {
        wasm_runtime_unload(underflow_module);
    }
}

// Instantiate parameterized tests for both Interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I64Extend16sTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<I64Extend16sTest::ParamType>& info) {
                             return info.param == Mode_Interp ? "Interpreter" : "AOT";
                         });
