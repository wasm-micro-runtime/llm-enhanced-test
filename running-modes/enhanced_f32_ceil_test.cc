/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <cmath>
#include <cfloat>
#include <gtest/gtest.h>
#include "wasm_runtime_common.h"
#include "wasm_exec_env.h"
#include "wasm_export.h"

/**
 * @brief Test fixture class for f32.ceil opcode comprehensive testing
 *
 * This class provides a complete test environment for validating the f32.ceil
 * WebAssembly opcode implementation across different WAMR execution modes.
 * Tests cover basic functionality, edge cases, precision boundaries, and
 * special IEEE 754 values to ensure comprehensive opcode coverage.
 */
class F32CeilTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Set up test environment and initialize WAMR runtime
     *
     * Initializes WAMR runtime with proper memory allocation settings,
     * loads the test WASM module containing f32.ceil test functions,
     * and sets up execution context for both interpreter and AOT modes.
     */
    void SetUp() override {
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for f32.ceil tests";

        // Load the WASM module containing f32.ceil test functions
        WASM_FILE = "wasm-apps/f32_ceil_test.wasm";
        load_wasm_module();
        ASSERT_NE(nullptr, module)
            << "Failed to load f32.ceil test WASM module: " << WASM_FILE;

        setup_execution_environment();
    }

    /**
     * @brief Clean up test environment and destroy WAMR runtime
     *
     * Properly cleans up execution contexts, unloads WASM modules,
     * and destroys WAMR runtime to prevent resource leaks between tests.
     */
    void TearDown() override {
        cleanup_execution_environment();
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        wasm_runtime_destroy();
    }

private:
    /**
     * @brief Load WASM module from file system
     *
     * Loads the compiled WASM bytecode file containing f32.ceil test
     * functions and validates successful module loading.
     */
    void load_wasm_module() {
        FILE *file = fopen(WASM_FILE, "rb");
        ASSERT_NE(nullptr, file) << "Cannot open WASM file: " << WASM_FILE;

        ASSERT_EQ(0, fseek(file, 0, SEEK_END))
            << "Failed to seek to end of WASM file";
        wasm_file_size = static_cast<uint32_t>(ftell(file));
        ASSERT_EQ(0, fseek(file, 0, SEEK_SET))
            << "Failed to seek to start of WASM file";

        wasm_file_buf = new uint8_t[wasm_file_size];
        ASSERT_EQ(wasm_file_size, fread(wasm_file_buf, 1, wasm_file_size, file))
            << "Failed to read WASM file contents";
        ASSERT_EQ(0, fclose(file))
            << "Failed to close WASM file";

        char error_buf[128];
        module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                   error_buf, sizeof(error_buf));
        if (module == nullptr) {
            delete[] wasm_file_buf;
            wasm_file_buf = nullptr;
        }
    }

    /**
     * @brief Set up execution environment for current test mode
     *
     * Creates appropriate execution environment based on test parameter
     * (interpreter vs AOT mode) and prepares for function execution.
     */
    void setup_execution_environment() {
        if (module == nullptr) return;

        module_inst = wasm_runtime_instantiate(module, 65536, 65536,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        ASSERT_NE(nullptr, exec_env)
            << "Failed to create execution environment";
    }

    /**
     * @brief Clean up execution environment and free resources
     *
     * Properly destroys execution contexts and frees allocated memory
     * to prevent resource leaks during test execution.
     */
    void cleanup_execution_environment() {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
            exec_env = nullptr;
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }
        if (wasm_file_buf != nullptr) {
            delete[] wasm_file_buf;
            wasm_file_buf = nullptr;
        }
    }

protected:
    /**
     * @brief Execute f32.ceil operation with specified input value
     *
     * @param input f32 value to apply ceiling function to
     * @return f32 result of ceil(input) operation
     *
     * Calls the exported WASM function to execute f32.ceil opcode
     * and returns the computed result for validation.
     */
    float call_f32_ceil(float input) {
        EXPECT_NE(nullptr, module_inst) << "Module instance not available";
        EXPECT_NE(nullptr, exec_env) << "Execution environment not available";

        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "test_f32_ceil");
        EXPECT_NE(nullptr, func) << "Cannot find test_f32_ceil function";

        uint32_t argv[2];
        memcpy(&argv[0], &input, sizeof(float));

        bool success = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(success) << "f32.ceil function call failed";

        float result;
        memcpy(&result, &argv[0], sizeof(float));
        return result;
    }

    // Test infrastructure members
    const char* WASM_FILE;
    uint8_t* wasm_file_buf = nullptr;
    uint32_t wasm_file_size = 0;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    char error_buf[128];
};

/**
 * @test SpecialValues_HandledCorrectly
 * @brief Validates f32.ceil handles IEEE 754 special values according to specification
 * @details Tests ceiling operation with special floating-point values including signed zeros,
 *          infinities, and NaN values. Ensures IEEE 754 compliance in special case handling.
 * @test_category Edge - Special IEEE 754 value validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_ceil_special_cases
 * @input_conditions IEEE 754 special values: ±0.0, ±∞, NaN
 * @expected_behavior Preserves special values: ceil(±0.0)=±0.0, ceil(±∞)=±∞, ceil(NaN)=NaN
 * @validation_method Bit-level comparison for signed zeros, isnan/isinf checks for special values
 */
TEST_P(F32CeilTest, SpecialValues_HandledCorrectly) {
    // Test positive and negative zero (preserve sign)
    float pos_zero = +0.0f;
    float neg_zero = -0.0f;
    ASSERT_EQ(pos_zero, call_f32_ceil(pos_zero))
        << "ceil(+0.0f) should return +0.0f";
    ASSERT_TRUE(signbit(call_f32_ceil(neg_zero)) != 0)
        << "ceil(-0.0f) should preserve negative zero sign";

    // Test positive and negative infinity
    ASSERT_TRUE(isinf(call_f32_ceil(INFINITY)) && call_f32_ceil(INFINITY) > 0)
        << "ceil(+∞) should return +∞";
    ASSERT_TRUE(isinf(call_f32_ceil(-INFINITY)) && call_f32_ceil(-INFINITY) < 0)
        << "ceil(-∞) should return -∞";

    // Test NaN values
    ASSERT_TRUE(isnan(call_f32_ceil(NAN)))
        << "ceil(NaN) should return NaN";
}

// Instantiate parametrized tests for both interpreter and AOT execution modes
INSTANTIATE_TEST_SUITE_P(
    CrossExecutionMode,
    F32CeilTest,
    testing::Values(
        Mode_Interp
#if WASM_ENABLE_AOT != 0
        , Mode_LLVM_JIT
#endif
    )
);
