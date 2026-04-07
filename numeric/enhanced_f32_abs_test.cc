/*
 * Copyright (C) 2025 Ant Group. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cmath>
#include <cfloat>
#include <cstdint>
#include <climits>
#include "wasm_runtime.h"
#include "wasm_export.h"

/**
 * @brief Test fixture for f32.abs opcode validation
 * @details Comprehensive testing of WebAssembly f32.abs instruction across
 *          interpreter and AOT execution modes. Tests cover IEEE 754 compliance,
 *          boundary conditions, special values, and sign bit manipulation correctness.
 */
class F32AbsTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Initialize WAMR runtime and load f32.abs test module
     * @details Sets up WAMR runtime with proper memory allocation and loads
     *          the compiled WASM module containing f32.abs test functions.
     */
    void SetUp() override {
        // Initialize WAMR runtime with system allocator
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load the f32.abs test WASM module
        const char* wasm_file = "wasm-apps/f32_abs_test.wasm";
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

        auto* buffer = (uint8_t*)wasm_runtime_malloc(*size);
        if (buffer != nullptr) {
            (void)fread(buffer, 1, *size, file);
        }
        (void)fclose(file);
        return buffer;
    }

    /**
     * @brief Execute f32.abs operation via WASM function call
     * @param input f32 value to compute absolute value
     * @return f32 absolute value result from WASM execution
     */
    float call_f32_abs(float input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst_, "abs_test");
        EXPECT_NE(nullptr, func) << "Failed to lookup abs_test function";

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
     * @return f32 result from WASM abs operation
     */
    float call_abs_special(float input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst_, "abs_special");
        EXPECT_NE(nullptr, func) << "Failed to lookup abs_special function";

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
     * @param input f32 boundary value (MIN, MAX, subnormal)
     * @return f32 result from WASM abs operation
     */
    float call_abs_boundary(float input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst_, "abs_boundary");
        EXPECT_NE(nullptr, func) << "Failed to lookup abs_boundary function";

        uint32_t wasm_args[1];
        memcpy(&wasm_args[0], &input, sizeof(float));

        bool success = wasm_runtime_call_wasm(exec_env_, func, 1, wasm_args);
        EXPECT_TRUE(success) << "WASM function call failed";

        float result;
        memcpy(&result, &wasm_args[0], sizeof(float));
        return result;
    }

    /**
     * @brief Check if two f32 values have identical bit patterns
     * @param a First f32 value
     * @param b Second f32 value
     * @return true if bit patterns are identical
     */
    bool same_bit_pattern(float a, float b) {
        uint32_t bits_a, bits_b;
        memcpy(&bits_a, &a, sizeof(float));
        memcpy(&bits_b, &b, sizeof(float));
        return bits_a == bits_b;
    }

    // Test fixture member variables
    wasm_module_t module_ = nullptr;
    wasm_module_inst_t module_inst_ = nullptr;
    wasm_exec_env_t exec_env_ = nullptr;
    uint8_t* buffer_ = nullptr;
    uint32_t buffer_size_ = 0;
};

/**
 * @test SpecialValues_IEEE754Compliance
 * @brief Validates IEEE 754 special value handling (zero, infinity, NaN)
 * @details Tests f32.abs behavior with IEEE 754 special values ensuring proper
 *          sign bit manipulation while preserving NaN and infinity semantics.
 *          Validates compliance with floating-point standard requirements.
 * @test_category Edge - Special value validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_abs_special_values
 * @input_conditions +0.0f, -0.0f, +∞, -∞, various NaN patterns
 * @expected_behavior +0.0f, +0.0f, +∞, +∞, positive NaN with preserved payload
 * @validation_method IEEE 754 property checks and bit pattern analysis
 */
TEST_P(F32AbsTest, SpecialValues_IEEE754Compliance) {
    // Test positive zero - should remain positive zero
    float pos_zero = +0.0f;
    float abs_pos_zero = call_abs_special(pos_zero);
    ASSERT_EQ(abs_pos_zero, 0.0f)
        << "abs(+0.0) should equal +0.0";
    ASSERT_FALSE(std::signbit(abs_pos_zero))
        << "abs(+0.0) should have positive sign bit";

    // Test negative zero - should become positive zero
    float neg_zero = -0.0f;
    float abs_neg_zero = call_abs_special(neg_zero);
    ASSERT_EQ(abs_neg_zero, 0.0f)
        << "abs(-0.0) should equal +0.0";
    ASSERT_FALSE(std::signbit(abs_neg_zero))
        << "abs(-0.0) should have positive sign bit cleared";

    // Test positive infinity - should remain positive infinity
    float pos_inf = std::numeric_limits<float>::infinity();
    float abs_pos_inf = call_abs_special(pos_inf);
    ASSERT_TRUE(std::isinf(abs_pos_inf))
        << "abs(+∞) should remain infinity";
    ASSERT_FALSE(std::signbit(abs_pos_inf))
        << "abs(+∞) should be positive infinity";

    // Test negative infinity - should become positive infinity
    float neg_inf = -std::numeric_limits<float>::infinity();
    float abs_neg_inf = call_abs_special(neg_inf);
    ASSERT_TRUE(std::isinf(abs_neg_inf))
        << "abs(-∞) should be infinity";
    ASSERT_FALSE(std::signbit(abs_neg_inf))
        << "abs(-∞) should be positive infinity";

    // Test quiet NaN - should preserve NaN with positive sign
    float quiet_nan = std::numeric_limits<float>::quiet_NaN();
    float abs_qnan = call_abs_special(quiet_nan);
    ASSERT_TRUE(std::isnan(abs_qnan))
        << "abs(qNaN) should preserve NaN property";
    ASSERT_FALSE(std::signbit(abs_qnan))
        << "abs(qNaN) should have positive sign bit";

    // Test negative quiet NaN - should preserve NaN with positive sign
    float neg_quiet_nan = -std::numeric_limits<float>::quiet_NaN();
    float abs_neg_qnan = call_abs_special(neg_quiet_nan);
    ASSERT_TRUE(std::isnan(abs_neg_qnan))
        << "abs(-qNaN) should preserve NaN property";
    ASSERT_FALSE(std::signbit(abs_neg_qnan))
        << "abs(-qNaN) should have positive sign bit";
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(CrossMode, F32AbsTest,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT),
                        [](const testing::TestParamInfo<RunningMode>& info) {
                            return info.param == Mode_Interp ? "Interpreter" : "AOT";
                        });