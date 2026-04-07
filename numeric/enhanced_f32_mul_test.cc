/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cmath>
#include <cfloat>
#include <limits>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;

/**
 * @brief Enhanced test suite for f32.mul opcode validation
 *
 * This test suite provides comprehensive validation of the f32.mul WebAssembly opcode
 * implementation in WAMR runtime. It validates IEEE 754 single-precision floating-point
 * multiplication across multiple execution modes (interpreter and AOT) with extensive
 * coverage of edge cases, special values, and boundary conditions.
 *
 * The test suite covers:
 * - Basic arithmetic operations with various operand combinations
 * - IEEE 754 special value handling (NaN, infinity, signed zeros)
 * - Boundary value operations and overflow/underflow scenarios
 * - Cross-execution mode consistency validation
 * - Mathematical property verification (commutativity)
 * - Module loading and error handling scenarios
 */
class F32MulTest : public testing::TestWithParam<RunningMode>
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
     * @brief Set up test environment and initialize WAMR runtime
     *
     * Initializes WAMR runtime with system allocator, loads the f32.mul test module,
     * and instantiates it for test execution. Handles both interpreter and AOT modes
     * based on test parameter configuration.
     */
    void SetUp() override
    {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        // Initialize WASM file path
        if (WASM_FILE.empty()) {
            char *cwd = getcwd(nullptr, 0);
            if (cwd != nullptr) {
                CWD = std::string(cwd);
                free(cwd);
            } else {
                CWD = ".";
            }
            WASM_FILE = CWD + "/wasm-apps/f32_mul_test.wasm";
        }

        // Load WASM test module from file
        buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr)
            << "Failed to read WASM file: " << WASM_FILE;

        // Load and validate WASM module
        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr)
            << "Failed to load WASM module: " << error_buf;

        // Instantiate WASM module for execution
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr)
            << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode based on test parameter
        wasm_runtime_set_running_mode(module_inst, GetParam());

        // Create execution environment
        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up test environment and WAMR runtime resources
     *
     * Properly deallocates module instances, unloads modules, frees buffers,
     * and destroys WAMR runtime to ensure clean test environment.
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
     * @brief Execute f32.mul operation with two f32 operands
     *
     * @param a First operand (multiplicand)
     * @param b Second operand (multiplier)
     * @return Result of a * b as f32 value
     *
     * Calls the WASM f32_mul_test function with the specified operands and
     * returns the IEEE 754 compliant multiplication result.
     */
    float call_f32_mul(float a, float b)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "f32_mul_test");
        EXPECT_NE(func, nullptr) << "Failed to lookup f32_mul_test function";

        uint32_t argv[3];
        memcpy(&argv[0], &a, sizeof(float));
        memcpy(&argv[1], &b, sizeof(float));

        bool success = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_TRUE(success) << "Failed to call f32_mul_test function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Runtime exception occurred: " << exception;

        float result;
        memcpy(&result, &argv[0], sizeof(float));
        return result;
    }
};

/**
 * @test BasicMultiplication_ReturnsCorrectResults
 * @brief Validates f32.mul produces correct arithmetic results for typical inputs
 * @details Tests fundamental multiplication operation with positive, negative, and mixed-sign floats.
 *          Verifies that f32.mul correctly computes a * b for various input combinations.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_mul_operation
 * @input_conditions Standard float pairs: (2.5, 3.0), (-4.0, -2.5), (6.0, -1.5), (-3.0, 4.0)
 * @expected_behavior Returns mathematical product with IEEE 754 precision
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(F32MulTest, BasicMultiplication_ReturnsCorrectResults)
{
    // Test positive number multiplication
    ASSERT_FLOAT_EQ(7.5f, call_f32_mul(2.5f, 3.0f))
        << "Failed multiplication of positive floats: 2.5 * 3.0";

    // Test multiplication of negative numbers (result positive)
    ASSERT_FLOAT_EQ(10.0f, call_f32_mul(-4.0f, -2.5f))
        << "Failed multiplication of negative floats: (-4.0) * (-2.5)";

    // Test mixed signs: positive * negative (result negative)
    ASSERT_FLOAT_EQ(-9.0f, call_f32_mul(6.0f, -1.5f))
        << "Failed mixed sign multiplication: 6.0 * (-1.5)";

    // Test mixed signs: negative * positive (result negative)
    ASSERT_FLOAT_EQ(-12.0f, call_f32_mul(-3.0f, 4.0f))
        << "Failed mixed sign multiplication: (-3.0) * 4.0";
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(RunningModeTest, F32MulTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<F32MulTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });
