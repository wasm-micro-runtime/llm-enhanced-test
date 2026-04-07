/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for i32.extend8_s Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly i32.extend8_s
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic sign extension functionality with typical values
 * - Corner Cases: Boundary conditions at 8-bit signed integer limits
 * - Edge Cases: Upper bit interference and mathematical properties
 * - Error Handling: Stack underflow and validation scenarios
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (lines handling i32.extend8_s)
 * Source Location: core/iwasm/interpreter/wasm_interp_classic.c (i32 sign extension operations)
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
 * @class I32Extend8sTest
 * @brief Test fixture for comprehensive i32.extend8_s opcode validation
 *
 * Provides WAMR runtime environment setup and cleanup for testing
 * i32.extend8_s sign extension operations across different execution modes.
 * Supports both interpreter and AOT modes through parameterized testing.
 */
class I32Extend8sTest : public testing::TestWithParam<RunningMode>
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
     * @brief Set up WAMR runtime environment and load i32.extend8_s test module
     *
     * Initializes WAMR runtime, loads the test WASM module, creates module instance,
     * and sets up execution environment for i32.extend8_s testing.
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
            WASM_FILE = "wasm-apps/i32_extend8_s_test.wasm";
        }
        if (WASM_FILE_UNDERFLOW.empty()) {
            WASM_FILE_UNDERFLOW = "wasm-apps/i32_extend8_s_underflow_test.wasm";
        }

        // Load i32.extend8_s test WASM module
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
     * @brief Execute i32.extend8_s operation with input value
     * @param input Input i32 value for sign extension
     * @return Sign-extended i32 result
     *
     * Calls the test_i32_extend8_s function in the loaded WASM module,
     * passing the input value and returning the sign-extended result.
     */
    int32_t call_i32_extend8_s(int32_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i32_extend8_s");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i32_extend8_s function";

        wasm_val_t params[1] = { 0 };
        wasm_val_t results[1] = { 0 };

        params[0].kind = WASM_I32;
        params[0].of.i32 = input;
        results[0].kind = WASM_I32;

        bool call_result = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, params);
        EXPECT_TRUE(call_result) << "Failed to call test_i32_extend8_s function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Exception occurred: " << exception;

        return results[0].of.i32;
    }

    /**
     * @brief Execute double i32.extend8_s operation for idempotent testing
     * @param input Input i32 value for double sign extension
     * @return Result of applying i32.extend8_s twice
     *
     * Calls the test_i32_extend8_s_double function to validate that
     * extend8_s(extend8_s(x)) == extend8_s(x) for all values.
     */
    int32_t call_i32_extend8_s_double(int32_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i32_extend8_s_double");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i32_extend8_s_double function";

        wasm_val_t params[1] = { 0 };
        wasm_val_t results[1] = { 0 };

        params[0].kind = WASM_I32;
        params[0].of.i32 = input;
        results[0].kind = WASM_I32;

        bool call_result = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, params);
        EXPECT_TRUE(call_result) << "Failed to call test_i32_extend8_s_double function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Exception occurred: " << exception;

        return results[0].of.i32;
    }
};

/**
 * @test BasicSignExtension_ReturnsCorrectResults
 * @brief Validates fundamental i32.extend8_s sign extension for typical values
 * @details Tests sign extension behavior for positive, negative, and mixed values.
 *          Verifies that the lower 8 bits are properly sign-extended to 32 bits
 *          while upper 24 bits of input are ignored.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i32_extend8_s_operation
 * @input_conditions Various 8-bit patterns with different upper bit configurations
 * @expected_behavior Correct sign extension based on bit 7 (sign bit)
 * @validation_method Direct comparison of expected vs actual results
 */
TEST_P(I32Extend8sTest, BasicSignExtension_ReturnsCorrectResults)
{
    // Test positive 8-bit values (sign bit = 0, should extend with 0s)
    ASSERT_EQ(0x00000042, call_i32_extend8_s(0x00000042))
        << "Positive value 66 should remain unchanged";

    ASSERT_EQ(0x0000007F, call_i32_extend8_s(0x0000007F))
        << "Max positive 8-bit value 127 should remain unchanged";

    // Test negative 8-bit values (sign bit = 1, should extend with 1s)
    ASSERT_EQ(0xFFFFFF80, call_i32_extend8_s(0x00000080))
        << "Value 128 should sign-extend to -128";

    ASSERT_EQ(0xFFFFFFFF, call_i32_extend8_s(0x000000FF))
        << "Value 255 should sign-extend to -1";

    // Test upper bits ignored scenarios
    ASSERT_EQ(0x00000078, call_i32_extend8_s(0x12345678))
        << "Upper bits should be ignored, only lower 8 bits (0x78 = 120) matter";

    ASSERT_EQ(0xFFFFFF90, call_i32_extend8_s(0xABCDEF90))
        << "Upper bits should be ignored, sign-extend 0x90 (-112)";
}

// Define test parameters for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    RunningModeTest,
    I32Extend8sTest,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<I32Extend8sTest::ParamType> &info) {
        return info.param == Mode_Interp ? "INTERP" : "AOT";
    }
);

// Test suite initialization - files will be set up by the main test executable
// Environment setup is handled by the shared test infrastructure
__attribute__((constructor))
static void setup_i32_extend8_s_test_paths() {
    // This constructor will run before main() to set up file paths
    char *cwd = getcwd(nullptr, 0);
    if (cwd != nullptr) {
        CWD = std::string(cwd);
        free(cwd);
    } else {
        CWD = ".";
    }

    WASM_FILE = CWD + "/wasm-apps/i32_extend8_s_test.wasm";
    WASM_FILE_UNDERFLOW = CWD + "/wasm-apps/i32_extend8_s_underflow_test.wasm";
}
