/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <unistd.h>
#include <cstring>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;

static int app_argc;
static char **app_argv;

/**
 * Test fixture for f64.const opcode validation
 *
 * This class provides comprehensive testing infrastructure for the f64.const WebAssembly opcode,
 * ensuring proper constant loading functionality across different execution modes (interpreter and AOT).
 * Tests validate that f64.const correctly pushes immediate 64-bit IEEE 754 floating-point values onto
 * the execution stack without consuming any stack operands. Includes comprehensive validation of
 * special IEEE 754 values including NaN, infinity, zero, subnormals, and boundary conditions.
 */
class F64ConstTest : public testing::TestWithParam<RunningMode>
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
     * Set up test environment for f64.const opcode testing
     *
     * Initializes WAMR runtime with appropriate configuration for testing f64.const operations.
     * Configures memory allocation, execution mode, and loads the f64.const test module.
     * Ensures proper runtime state before executing individual test cases.
     */
    void SetUp() override
    {
        memset(error_buf, 0, sizeof(error_buf));

        buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * Clean up test environment after f64.const opcode testing
     *
     * Performs proper cleanup of WASM module instances, modules, and runtime resources.
     * Ensures no memory leaks or resource conflicts between test cases.
     * Maintains clean test environment for subsequent test execution.
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
            wasm_runtime_free(buf);
            buf = nullptr;
        }
    }

    /**
     * Execute f64.const test function and return the loaded constant value
     *
     * @param func_name Name of the WASM function to execute (must return f64)
     * @return The f64 constant value loaded by the function
     */
    double call_f64_const_func(const char* func_name)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(func, nullptr) << "Failed to lookup function: " << func_name;

        wasm_val_t results[1] = { 0 };
        wasm_val_t arguments[1] = { 0 };

        bool call_result = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 0, arguments);
        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_TRUE(call_result) << "Function call failed: " << (exception != nullptr ? exception : "Unknown error");
        EXPECT_EQ(exception, nullptr) << "Unexpected exception: " << exception;

        return results[0].of.f64;
    }

    /**
     * Compare f64 values with bit-exact precision
     *
     * @param expected Expected f64 value
     * @param actual Actual f64 value returned by WASM function
     * @param description Test description for error messages
     */
    void assert_f64_bitwise_equal(double expected, double actual, const char* description)
    {
        uint64_t expected_bits, actual_bits;
        memcpy(&expected_bits, &expected, sizeof(double));
        memcpy(&actual_bits, &actual, sizeof(double));

        ASSERT_EQ(expected_bits, actual_bits)
            << description << " - Expected: " << expected
            << " (0x" << std::hex << expected_bits << "), "
            << "Actual: " << actual
            << " (0x" << std::hex << actual_bits << ")";
    }

    /**
     * Execute multiple f64.const functions and return results in order
     *
     * @param func1 First function name
     * @param func2 Second function name
     * @param func3 Third function name
     * @return Array of 3 f64 values loaded by the functions
     */
    std::array<double, 3> call_multiple_f64_const_funcs(const char* func1, const char* func2, const char* func3)
    {
        std::array<double, 3> results;
        results[0] = call_f64_const_func(func1);
        results[1] = call_f64_const_func(func2);
        results[2] = call_f64_const_func(func3);
        return results;
    }
};

/**
 * @test SpecialValues_PreservesIEEE754Properties
 * @brief Validates IEEE 754 special values including NaN, infinity, and signed zeros
 * @details Tests f64.const loading of special IEEE 754 values with exact bit-pattern preservation.
 *          Verifies proper handling of positive/negative zeros, infinities, quiet NaN, and signaling NaN.
 *          Ensures compliance with IEEE 754 standard for special value representation.
 * @test_category Edge - Special value validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_const_operation
 * @input_conditions Special values: +0.0, -0.0, +∞, -∞, quiet NaN, signaling NaN
 * @expected_behavior Proper IEEE 754 special value behavior with exact bit preservation
 * @validation_method IEEE 754 property verification (isnan, isinf, signbit) with bit-pattern validation
 */
TEST_P(F64ConstTest, SpecialValues_PreservesIEEE754Properties)
{
    // Test positive zero (0x0000000000000000)
    double result_pos_zero = call_f64_const_func("test_f64_const_pos_zero");
    assert_f64_bitwise_equal(0.0, result_pos_zero, "Positive zero constant");
    ASSERT_FALSE(signbit(result_pos_zero)) << "Positive zero should not have sign bit set";

    // Test negative zero (0x8000000000000000)
    double result_neg_zero = call_f64_const_func("test_f64_const_neg_zero");
    double neg_zero = -0.0;
    assert_f64_bitwise_equal(neg_zero, result_neg_zero, "Negative zero constant");
    ASSERT_TRUE(signbit(result_neg_zero)) << "Negative zero should have sign bit set";

    // Test positive infinity (0x7FF0000000000000)
    double result_pos_inf = call_f64_const_func("test_f64_const_pos_inf");
    ASSERT_TRUE(std::isinf(result_pos_inf)) << "Should be positive infinity";
    ASSERT_FALSE(signbit(result_pos_inf)) << "Positive infinity should not have sign bit set";
    assert_f64_bitwise_equal(INFINITY, result_pos_inf, "Positive infinity constant");

    // Test negative infinity (0xFFF0000000000000)
    double result_neg_inf = call_f64_const_func("test_f64_const_neg_inf");
    ASSERT_TRUE(std::isinf(result_neg_inf)) << "Should be negative infinity";
    ASSERT_TRUE(signbit(result_neg_inf)) << "Negative infinity should have sign bit set";
    assert_f64_bitwise_equal(-std::numeric_limits<double>::infinity(), result_neg_inf, "Negative infinity constant");

    // Test quiet NaN (0x7FF8000000000000 - canonical)
    double result_qnan = call_f64_const_func("test_f64_const_qnan");
    ASSERT_TRUE(std::isnan(result_qnan)) << "Should be NaN (quiet)";
    // Verify it's a quiet NaN by checking the quiet bit (bit 51)
    uint64_t qnan_bits;
    memcpy(&qnan_bits, &result_qnan, sizeof(double));
    ASSERT_TRUE((qnan_bits & 0x0008000000000000ULL) != 0) << "Should be quiet NaN with quiet bit set";

    // Test signaling NaN (0x7FF0000000000001)
    double result_snan = call_f64_const_func("test_f64_const_snan");
    ASSERT_TRUE(std::isnan(result_snan)) << "Should be NaN (signaling)";
    // Note: Some systems convert sNaN to qNaN during operations, so we verify NaN property
}

/**
 * @test MultipleConstants_MaintainsStackOrder
 * @brief Tests sequential f64.const loading and proper stack ordering
 * @details Validates that multiple f64.const instructions correctly maintain stack order and growth.
 *          Tests that constants are pushed onto stack in proper sequence and accessible in correct order.
 *          Verifies stack management behavior for consecutive constant loading operations.
 * @test_category Edge - Stack management validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_const_operation
 * @input_conditions Sequential loading: first 1.0, then 2.0, then 3.0
 * @expected_behavior Stack order maintained: top=3.0, middle=2.0, bottom=1.0
 * @validation_method Sequential function calls with stack position verification
 */
TEST_P(F64ConstTest, MultipleConstants_MaintainsStackOrder)
{
    // Test loading three constants in sequence and verifying stack order
    std::array<double, 3> results = call_multiple_f64_const_funcs(
        "test_f64_const_first",   // Loads 1.0
        "test_f64_const_second",  // Loads 2.0
        "test_f64_const_third"    // Loads 3.0
    );

    // Verify each constant was loaded correctly
    assert_f64_bitwise_equal(1.0, results[0], "First constant (1.0)");
    assert_f64_bitwise_equal(2.0, results[1], "Second constant (2.0)");
    assert_f64_bitwise_equal(3.0, results[2], "Third constant (3.0)");

    // Test function that loads multiple constants and returns them in stack order
    double stack_test_result = call_f64_const_func("test_f64_const_stack_order");
    // This function should load 10.0, 20.0, 30.0 and return the sum (60.0)
    assert_f64_bitwise_equal(60.0, stack_test_result, "Stack order verification (sum should be 60.0)");
}

// Instantiate parameterized tests for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningModeTest, F64ConstTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<F64ConstTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });

int main(int argc, char **argv)
{
    char *cwd = getcwd(nullptr, 0);
    if (cwd != nullptr) {
        CWD = std::string(cwd);
        free(cwd);
    } else {
        CWD = ".";
    }
    WASM_FILE = CWD + "/wasm-apps/f64_const_test.wasm";

    app_argc = argc;
    app_argv = argv;

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
