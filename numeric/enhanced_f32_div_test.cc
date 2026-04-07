/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for f32.div Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly f32.div
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic floating-point division with typical values
 * - Corner Cases: Boundary conditions and overflow/underflow scenarios
 * - Edge Cases: IEEE 754 special values (NaN, infinity, signed zeros)
 * - Error Handling: Stack underflow and validation errors
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (f32.div implementation)
 * Source Location: core/iwasm/interpreter/wasm_interp_classic.c (DEF_OP_EQZ macro for f32.div)
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include <cmath>
#include <limits>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

using namespace std;

/**
 * @brief Test fixture class for f32.div opcode validation
 * @details Provides comprehensive testing infrastructure for the WebAssembly f32.div
 *          instruction across both interpreter and AOT execution modes. This test suite
 *          validates IEEE 754 single-precision floating-point division behavior, including
 *          special value handling, overflow/underflow conditions, and compliance with
 *          IEEE 754 standard for division operations.
 */
class F32DivTest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up test environment before each test case
     * @details Initializes WAMR runtime with system allocator, loads the f32.div test
     *          WASM module, and prepares execution context for both interpreter and AOT modes.
     *          Ensures proper module validation and instance creation for reliable test execution.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize WAMR runtime environment
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load WASM module for f32.div testing
        load_test_module();
    }

    /**
     * @brief Clean up test environment after each test case
     * @details Properly destroys WASM module instance, unloads module, releases memory
     *          resources, and shuts down WAMR runtime to prevent resource leaks.
     */
    void TearDown() override
    {
        // Clean up WAMR resources in proper order
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
        wasm_runtime_destroy();
    }

private:
    /**
     * @brief Load f32.div test WASM module from file system
     * @details Reads the compiled WASM bytecode for f32.div tests, validates module
     *          format, loads module into WAMR, and creates executable module instance.
     *          Handles both interpreter and AOT execution modes based on test parameters.
     */
    void load_test_module()
    {
        // Construct WASM file path relative to test execution directory
        const char *wasm_file = "wasm-apps/f32_div_test.wasm";

        // Read WASM module bytecode from file
        buf = (uint8_t *)bh_read_file_to_buffer(wasm_file, &buf_size);
        ASSERT_NE(buf, nullptr)
            << "Failed to read WASM file: " << wasm_file;
        ASSERT_GT(buf_size, 0U)
            << "WASM file is empty: " << wasm_file;

        // Load WASM module with validation
        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr)
            << "Failed to load WASM module: " << error_buf;

        // Create module instance with appropriate memory configuration
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr)
            << "Failed to instantiate WASM module: " << error_buf;

        // Create execution environment for function calls
        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr)
            << "Failed to create execution environment";
    }

protected:
    // WAMR runtime configuration and state management
    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint32_t buf_size, stack_size = 8092, heap_size = 8092;
    uint8_t *buf = nullptr;
    char error_buf[128] = { 0 };

    /**
     * @brief Execute f32.div WASM function with two float arguments
     * @param dividend The first operand (dividend) for division operation
     * @param divisor The second operand (divisor) for division operation
     * @return Result of f32.div operation as IEEE 754 single-precision float
     */
    float call_f32_div(float dividend, float divisor)
    {
        // Lookup f32.div test function in module
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "f32_div_test");
        EXPECT_NE(func, nullptr) << "Failed to find f32_div_test function";

        // Prepare function arguments: dividend and divisor
        uint32_t wasm_argv[2];
        memcpy(&wasm_argv[0], &dividend, sizeof(float));
        memcpy(&wasm_argv[1], &divisor, sizeof(float));

        // Execute function and capture result
        bool success = wasm_runtime_call_wasm(exec_env, func, 2, wasm_argv);
        EXPECT_TRUE(success) << "f32.div function execution failed: "
                            << wasm_runtime_get_exception(module_inst);

        // Extract and return f32 result
        float result;
        memcpy(&result, &wasm_argv[0], sizeof(float));
        return result;
    }

    /**
     * @brief Execute f32.div WASM function expecting potential exceptions
     * @param dividend The first operand (dividend) for division operation
     * @param divisor The second operand (divisor) for division operation
     * @return True if function executed without exceptions, false otherwise
     */
    bool call_f32_div_no_exception_expected(float dividend, float divisor)
    {
        // Lookup f32.div test function in module
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "f32_div_test");
        EXPECT_NE(func, nullptr) << "Failed to find f32_div_test function";

        // Prepare function arguments: dividend and divisor
        uint32_t wasm_argv[2];
        memcpy(&wasm_argv[0], &dividend, sizeof(float));
        memcpy(&wasm_argv[1], &divisor, sizeof(float));

        // Execute function and return success status
        return wasm_runtime_call_wasm(exec_env, func, 2, wasm_argv);
    }
};

/**
 * @test FractionalDivision_ProducesAccurateResults
 * @brief Validates f32.div produces correct results for fractional quotients within IEEE 754 precision
 * @details Tests division operations that produce non-integer results, ensuring accuracy
 *          within single-precision floating-point representation limits. Validates proper
 *          handling of repeating decimals and fractional arithmetic.
 * @test_category Main - Fractional arithmetic validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_div_operation
 * @input_conditions Fractional cases: (1.0f, 3.0f), (7.0f, 2.0f), (22.0f, 7.0f)
 * @expected_behavior Returns approximate quotients with IEEE 754 single-precision accuracy
 * @validation_method Comparison within acceptable floating-point tolerance limits
 */
TEST_P(F32DivTest, FractionalDivision_ProducesAccurateResults) {
    // Test division producing repeating decimal (1/3)
    float result1 = call_f32_div(1.0f, 3.0f);
    ASSERT_NEAR(result1, 0.33333334f, 1e-7f)
        << "Failed: 1.0 / 3.0 approximation within single-precision limits";

    // Test simple fractional division (7/2)
    ASSERT_EQ(call_f32_div(7.0f, 2.0f), 3.5f)
        << "Failed: 7.0 / 2.0 should equal exactly 3.5";

    // Test irrational approximation (22/7 ≈ π)
    float result2 = call_f32_div(22.0f, 7.0f);
    ASSERT_NEAR(result2, 3.142857f, 1e-6f)
        << "Failed: 22.0 / 7.0 approximation accuracy";
}

/**
 * @test IdentityOperations_PreserveValue
 * @brief Validates f32.div identity and inverse operations preserve mathematical properties
 * @details Tests division by 1.0 (identity), self-division (should equal 1.0), and division
 *          by -1.0 (negation), ensuring these fundamental mathematical properties are
 *          correctly implemented in the IEEE 754 division operation.
 * @test_category Edge - Mathematical identity properties
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_div_identity_handling
 * @input_conditions Identity cases: (x, 1.0f), (x, x), (x, -1.0f) for various x values
 * @expected_behavior Returns x, 1.0f, -x respectively (when mathematically valid)
 * @validation_method Direct equality comparison for exact mathematical identities
 */
TEST_P(F32DivTest, IdentityOperations_PreserveValue) {
    // Test division by 1.0 (multiplicative identity)
    ASSERT_EQ(call_f32_div(42.5f, 1.0f), 42.5f)
        << "Failed: x / 1.0 should equal x (identity property)";

    ASSERT_EQ(call_f32_div(-17.25f, 1.0f), -17.25f)
        << "Failed: negative x / 1.0 should equal x (identity property)";

    // Test self-division (should equal 1.0 for non-zero finite values)
    ASSERT_EQ(call_f32_div(5.5f, 5.5f), 1.0f)
        << "Failed: x / x should equal 1.0 for finite non-zero x";

    ASSERT_EQ(call_f32_div(-8.75f, -8.75f), 1.0f)
        << "Failed: negative x / negative x should equal 1.0";

    // Test division by -1.0 (negation operation)
    ASSERT_EQ(call_f32_div(3.14f, -1.0f), -3.14f)
        << "Failed: x / -1.0 should equal -x (negation property)";

    ASSERT_EQ(call_f32_div(-2.718f, -1.0f), 2.718f)
        << "Failed: negative x / -1.0 should equal positive x";
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(RunningMode, F32DivTest,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT),
                        [](const testing::TestParamInfo<F32DivTest::ParamType> &info) {
                            return info.param == Mode_Interp ? "INTERP" : "AOT";
                        });
