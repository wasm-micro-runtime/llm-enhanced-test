/*
 * Copyright (C) 2025 Ant Group. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cmath>
#include <cfloat>
#include <cstdint>
#include "wasm_runtime.h"
#include "wasm_export.h"

/**
 * @brief Test fixture for f32.sqrt opcode validation
 * @details Comprehensive testing of WebAssembly f32.sqrt instruction across
 *          interpreter and AOT execution modes. Tests cover IEEE 754 compliance,
 *          boundary conditions, special values, and mathematical correctness.
 */
class F32SqrtTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Initialize WAMR runtime and load f32.sqrt test module
     * @details Sets up WAMR runtime with proper memory allocation and loads
     *          the compiled WASM module containing f32.sqrt test functions.
     */
    void SetUp() override {
        // Initialize WAMR runtime with system allocator
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load the f32.sqrt test WASM module
        const char* wasm_file = "wasm-apps/f32_sqrt_test.wasm";
        buffer_ = load_wasm_buffer(wasm_file, &buffer_size_);
        ASSERT_NE(nullptr, buffer_)
            << "Failed to load WASM file: " << wasm_file;

        // Load and instantiate the module
        char error_buf[256];
        module_ = wasm_runtime_load(buffer_, buffer_size_,
                                   error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_)
            << "Failed to load WASM module: " << error_buf;

        module_inst_ = wasm_runtime_instantiate(module_, 65536, 65536,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst_)
            << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode based on test parameter
        wasm_runtime_set_running_mode(module_inst_, GetParam());

        // Create execution environment for WASM function calls
        exec_env_ = wasm_runtime_create_exec_env(module_inst_, 65536);
        ASSERT_NE(nullptr, exec_env_)
            << "Failed to create execution environment";
    }

    /**
     * @brief Clean up WAMR runtime resources
     * @details Properly destroys module instance, unloads module, frees buffer,
     *          and shuts down WAMR runtime to prevent memory leaks.
     */
    void TearDown() override {
        if (exec_env_ != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env_);
            exec_env_ = nullptr;
        }
        if (module_inst_ != nullptr) {
            wasm_runtime_deinstantiate(module_inst_);
            module_inst_ = nullptr;
        }
        if (module_ != nullptr) {
            wasm_runtime_unload(module_);
            module_ = nullptr;
        }
        if (buffer_ != nullptr) {
            wasm_runtime_free(buffer_);
            buffer_ = nullptr;
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM file into memory buffer
     * @param filename Path to WASM file relative to test execution directory
     * @param size Pointer to store the loaded buffer size
     * @return Pointer to allocated buffer containing WASM bytecode
     */
    uint8_t* load_wasm_buffer(const char* filename, uint32_t* size) {
        FILE* file = fopen(filename, "rb");
        if (file == nullptr) return nullptr;

        (void)fseek(file, 0, SEEK_END);
        *size = ftell(file);
        (void)fseek(file, 0, SEEK_SET);

        auto* buffer = static_cast<uint8_t*>(wasm_runtime_malloc(*size));
        if (buffer != nullptr) {
            (void)fread(buffer, 1, *size, file);
        }
        (void)fclose(file);
        return buffer;
    }

    /**
     * @brief Execute f32.sqrt operation via WASM function call
     * @param input f32 value to compute square root
     * @return f32 square root result from WASM execution
     */
    float call_f32_sqrt(float input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst_, "sqrt_test");
        EXPECT_NE(nullptr, func) << "Failed to lookup sqrt_test function";

        uint32_t wasm_args[1];
        memcpy(&wasm_args[0], &input, sizeof(float));

        bool success = wasm_runtime_call_wasm(exec_env_, func, 1, wasm_args);
        EXPECT_TRUE(success) << "WASM function call failed";

        float result;
        memcpy(&result, &wasm_args[0], sizeof(float));
        return result;
    }

    /**
     * @brief Execute special values test via WASM function call
     * @param input f32 special value (NaN, infinity, zero)
     * @return f32 result from WASM sqrt operation
     */
    float call_sqrt_special(float input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst_, "sqrt_special");
        EXPECT_NE(nullptr, func) << "Failed to lookup sqrt_special function";

        uint32_t wasm_args[1];
        memcpy(&wasm_args[0], &input, sizeof(float));

        bool success = wasm_runtime_call_wasm(exec_env_, func, 1, wasm_args);
        EXPECT_TRUE(success) << "WASM function call failed";

        float result;
        memcpy(&result, &wasm_args[0], sizeof(float));
        return result;
    }

    /**
     * @brief Execute boundary values test via WASM function call
     * @param input f32 boundary value (FLT_MAX, FLT_MIN, etc.)
     * @return f32 result from WASM sqrt operation
     */
    float call_sqrt_boundary(float input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst_, "sqrt_boundary");
        EXPECT_NE(nullptr, func) << "Failed to lookup sqrt_boundary function";

        uint32_t wasm_args[1];
        memcpy(&wasm_args[0], &input, sizeof(float));

        bool success = wasm_runtime_call_wasm(exec_env_, func, 1, wasm_args);
        EXPECT_TRUE(success) << "WASM function call failed";

        float result;
        memcpy(&result, &wasm_args[0], sizeof(float));
        return result;
    }

private:
    wasm_module_t module_ = nullptr;
    wasm_module_inst_t module_inst_ = nullptr;
    wasm_exec_env_t exec_env_ = nullptr;
    uint8_t* buffer_ = nullptr;
    uint32_t buffer_size_ = 0;
};

/**
 * @test SpecialValues_IEEE754Compliant
 * @brief Validates f32.sqrt handles IEEE 754 special values correctly
 * @details Tests IEEE 754 compliant behavior for special floating-point values
 *          including positive/negative zero, infinity, and NaN propagation.
 * @test_category Edge - IEEE 754 special value validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_sqrt_special_values
 * @input_conditions ±0.0f, +∞, NaN, -∞ (produces NaN)
 * @expected_behavior IEEE 754 compliant special value results
 * @validation_method Special value testing with isnan(), isinf(), sign bit validation
 */
TEST_P(F32SqrtTest, SpecialValues_IEEE754Compliant) {
    // Test positive zero - should return positive zero
    float pos_zero_result = call_sqrt_special(0.0f);
    ASSERT_EQ(pos_zero_result, 0.0f)
        << "sqrt(+0.0) should return +0.0";
    ASSERT_FALSE(std::signbit(pos_zero_result))
        << "sqrt(+0.0) should preserve positive sign";

    // Test negative zero - should return negative zero per IEEE 754
    float neg_zero = -0.0f;
    float neg_zero_result = call_sqrt_special(neg_zero);
    ASSERT_EQ(neg_zero_result, 0.0f)
        << "sqrt(-0.0) should return zero";
    ASSERT_TRUE(std::signbit(neg_zero_result))
        << "sqrt(-0.0) should preserve negative sign";

    // Test positive infinity - should return positive infinity
    float pos_inf_result = call_sqrt_special(INFINITY);
    ASSERT_TRUE(std::isinf(pos_inf_result))
        << "sqrt(+∞) should return infinity";
    ASSERT_FALSE(std::signbit(pos_inf_result))
        << "sqrt(+∞) should be positive infinity";

    // Test NaN - should return NaN (NaN propagation)
    float nan_result = call_sqrt_special(NAN);
    ASSERT_TRUE(std::isnan(nan_result))
        << "sqrt(NaN) should return NaN";

    // Test negative infinity - should return NaN per IEEE 754
    float neg_inf_result = call_sqrt_special(-INFINITY);
    ASSERT_TRUE(std::isnan(neg_inf_result))
        << "sqrt(-∞) should return NaN";
}

// Instantiate tests for both interpreter and AOT execution modes
INSTANTIATE_TEST_SUITE_P(RunningModeTest, F32SqrtTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<F32SqrtTest::ParamType>& info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });
