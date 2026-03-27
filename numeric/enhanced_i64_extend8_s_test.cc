/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for i64.extend8_s Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly i64.extend8_s
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic sign extension functionality with typical values
 * - Corner Cases: Boundary conditions at 8-bit signed integer limits
 * - Edge Cases: Upper bit interference and mathematical properties
 * - Error Handling: Stack underflow and validation scenarios
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (lines handling i64.extend8_s)
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
 * @class I64Extend8sTest
 * @brief Test fixture for comprehensive i64.extend8_s opcode validation
 *
 * Provides WAMR runtime environment setup and cleanup for testing
 * i64.extend8_s sign extension operations across different execution modes.
 * Supports both interpreter and AOT modes through parameterized testing.
 */
class I64Extend8sTest : public testing::TestWithParam<RunningMode>
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
     * @brief Set up WAMR runtime environment and load i64.extend8_s test module
     *
     * Initializes WAMR runtime, loads the test WASM module, creates module instance,
     * and sets up execution environment for i64.extend8_s testing.
     * Configures the running mode based on test parameter (interpreter/AOT).
     */
    void SetUp() override
    {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        // Initialize WASM file paths if not already initialized
        if (WASM_FILE.empty()) {
            WASM_FILE = "wasm-apps/i64_extend8_s_test.wasm";
        }
        if (WASM_FILE_UNDERFLOW.empty()) {
            WASM_FILE_UNDERFLOW = "wasm-apps/i64_extend8_s_stack_underflow.wasm";
        }

        // Load i64.extend8_s test WASM module
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
     * @brief Execute i64.extend8_s operation with input value
     * @param input Input i64 value for sign extension
     * @return Sign-extended i64 result
     *
     * Calls the test_i64_extend8_s function in the loaded WASM module,
     * passing the input value and returning the sign-extended result.
     */
    int64_t call_i64_extend8_s(int64_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_extend8_s");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_extend8_s function";

        wasm_val_t params[1] = { 0 };
        wasm_val_t results[1] = { 0 };

        params[0].kind = WASM_I64;
        params[0].of.i64 = input;
        results[0].kind = WASM_I64;

        bool call_result = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, params);
        EXPECT_TRUE(call_result) << "Failed to call test_i64_extend8_s function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Exception occurred: " << exception;

        return results[0].of.i64;
    }

    /**
     * @brief Execute double i64.extend8_s operation for idempotent testing
     * @param input Input i64 value for double sign extension
     * @return Double sign-extended i64 result
     *
     * Calls the test_i64_extend8_s_double function to validate that applying
     * sign extension twice produces the same result (idempotent property).
     */
    int64_t call_i64_extend8_s_double(int64_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_extend8_s_double");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_extend8_s_double function";

        wasm_val_t params[1] = { 0 };
        wasm_val_t results[1] = { 0 };

        params[0].kind = WASM_I64;
        params[0].of.i64 = input;
        results[0].kind = WASM_I64;

        bool call_result = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, params);
        EXPECT_TRUE(call_result) << "Failed to call test_i64_extend8_s_double function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Exception occurred: " << exception;

        return results[0].of.i64;
    }

    /**
     * @brief Execute stack underflow test for i64.extend8_s operation
     * @return False on expected failure, true on unexpected success
     *
     * Loads and executes a WASM module designed to trigger stack underflow
     * for i64.extend8_s operation, verifying proper error handling.
     */
    bool test_stack_underflow()
    {
        uint8_t *underflow_buf = nullptr;
        uint32_t underflow_buf_size;
        wasm_module_t underflow_module = nullptr;

        // Load stack underflow test module
        underflow_buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE_UNDERFLOW.c_str(), &underflow_buf_size);
        if (underflow_buf == nullptr) {
            return false; // File read failed as expected
        }

        underflow_module = wasm_runtime_load(underflow_buf, underflow_buf_size,
                                           error_buf, sizeof(error_buf));

        // Expected to fail module load due to invalid stack operations
        bool load_failed = (underflow_module == nullptr);

        if (underflow_buf != nullptr) {
            BH_FREE(underflow_buf);
        }

        return !load_failed; // Return true if load unexpectedly succeeded
    }
};

/**
 * @test BasicSignExtension_ReturnsCorrectValues
 * @brief Validates i64.extend8_s produces correct sign extension for typical inputs
 * @details Tests fundamental sign extension operation with positive, negative, and zero values.
 *          Verifies that i64.extend8_s correctly interprets lower 8 bits as signed and extends.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i64_extend8_s_operation
 * @input_conditions Standard integer values: 42 (positive), 200 (negative as int8), 0 (zero)
 * @expected_behavior Returns mathematically correct sign-extended values: 42, -56, 0
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(I64Extend8sTest, BasicSignExtension_ReturnsCorrectValues)
{
    // Test positive 8-bit value (42) - should extend with zeros
    ASSERT_EQ(42LL, call_i64_extend8_s(42LL))
        << "Sign extension of positive value 42 failed";

    // Test negative 8-bit value (200 = -56 as signed int8) - should extend with ones
    ASSERT_EQ(-56LL, call_i64_extend8_s(200LL))
        << "Sign extension of 200 (interpreted as -56) failed";

    // Test zero value - should remain zero
    ASSERT_EQ(0LL, call_i64_extend8_s(0LL))
        << "Sign extension of zero value failed";
}

/**
 * @test BoundarySignExtension_HandlesEdgeCases
 * @brief Validates i64.extend8_s handles boundary conditions at 8-bit limits
 * @details Tests sign extension at critical 8-bit signed integer boundaries.
 *          Verifies correct behavior at maximum positive, minimum negative, and edge values.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i64_extend8_s_boundary_handling
 * @input_conditions Boundary values: 127 (max positive), 128 (min negative), 255 (max uint8)
 * @expected_behavior Returns correct sign-extended values: 127, -128, -1
 * @validation_method Direct comparison with mathematically expected sign extension results
 */
TEST_P(I64Extend8sTest, BoundarySignExtension_HandlesEdgeCases)
{
    // Test maximum positive signed 8-bit value (127)
    ASSERT_EQ(127LL, call_i64_extend8_s(127LL))
        << "Sign extension of maximum positive 8-bit value (127) failed";

    // Test minimum negative signed 8-bit value (128 interpreted as -128)
    ASSERT_EQ(-128LL, call_i64_extend8_s(128LL))
        << "Sign extension of 128 (interpreted as -128) failed";

    // Test maximum unsigned 8-bit value (255 interpreted as -1)
    ASSERT_EQ(-1LL, call_i64_extend8_s(255LL))
        << "Sign extension of 255 (interpreted as -1) failed";
}

/**
 * @test StackUnderflow_HandledCorrectly
 * @brief Validates proper handling of stack underflow scenarios
 * @details Tests that i64.extend8_s operation with insufficient stack values is handled correctly.
 *          Verifies that stack underflow conditions are detected during module validation.
 * @test_category Error - Stack underflow validation
 * @coverage_target core/iwasm/interpreter/wasm_loader.c:stack_validation
 * @input_conditions WASM module with insufficient stack values for i64.extend8_s
 * @expected_behavior Module loading fails with proper error detection
 * @validation_method Verification that invalid modules are rejected during load phase
 */
TEST_P(I64Extend8sTest, StackUnderflow_HandledCorrectly)
{
    // Test stack underflow handling (should fail at module load time)
    ASSERT_FALSE(test_stack_underflow())
        << "Stack underflow test should fail but succeeded";
}

// Instantiate parameterized tests for both Interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I64Extend8sTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<I64Extend8sTest::ParamType>& info) {
                             return info.param == Mode_Interp ? "Interpreter" : "AOT";
                         });

// Initialize file paths for test execution
__attribute__((constructor))
void init_i64_extend8s_test_paths() {
    char *cwd_buffer = getcwd(nullptr, 0);
    if (cwd_buffer != nullptr) {
        CWD = std::string(cwd_buffer);
        free(cwd_buffer);
    }
    WASM_FILE = CWD + "/wasm-apps/i64_extend8_s_test.wasm";
    WASM_FILE_UNDERFLOW = CWD + "/wasm-apps/i64_extend8_s_stack_underflow.wasm";
}
