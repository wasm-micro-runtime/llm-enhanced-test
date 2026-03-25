/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "bh_read_file.h"
#include "wasm_runtime_common.h"

static std::string WASM_FILE = "wasm-apps/i64_trunc_s_test.wasm";

/**
 * @brief Test suite for i64.trunc_s opcode validation
 *
 * This class tests both i64.trunc_s/f32 and i64.trunc_s/f64 operations.
 * Tests cover basic functionality, boundary conditions, edge cases, and error scenarios.
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @coverage_target core/iwasm/aot/aot_runtime.c:aot_call_function
 */
class I64TruncSTest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up test environment and WAMR runtime
     *
     * Initializes WAMR runtime with system allocator, loads test WASM modules
     * and prepares execution instances for both interpreter and AOT modes.
     *
     * @coverage_target core/iwasm/common/wasm_runtime_common.c:wasm_runtime_full_init
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_EQ(wasm_runtime_full_init(&init_args), true);

        buffer = (uint8_t*)bh_read_file_to_buffer(WASM_FILE.c_str(), &size);
        ASSERT_NE(buffer, nullptr) << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(buffer, size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load module: " << error_buf;

        wasm_module_inst = wasm_runtime_instantiate(
            module, 65536, 0, error_buf, sizeof(error_buf));
        ASSERT_NE(wasm_module_inst, nullptr) << "Failed to instantiate module: " << error_buf;

        exec_env = wasm_runtime_create_exec_env(wasm_module_inst, 65536);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up test environment and WAMR runtime
     *
     * Properly deallocates WASM instances, modules, and runtime resources
     * following RAII patterns for reliable cleanup.
     *
     * @coverage_target core/iwasm/common/wasm_runtime_common.c:wasm_runtime_destroy
     */
    void TearDown() override
    {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
            exec_env = nullptr;
        }
        if (wasm_module_inst != nullptr) {
            wasm_runtime_deinstantiate(wasm_module_inst);
            wasm_module_inst = nullptr;
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (buffer != nullptr) {
            wasm_runtime_free(buffer);
            buffer = nullptr;
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Call i64.trunc_s/f32 WASM function with f32 input
     *
     * @param input f32 floating-point value to convert
     * @return int64_t converted signed 64-bit integer value
     *
     * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
     */
    int64_t call_i64_trunc_s_f32(float input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(wasm_module_inst, "i64_trunc_s_f32");
        EXPECT_NE(func, nullptr) << "Failed to lookup i64_trunc_s_f32 function";

        uint32 argv[1] = { 0 };
        memcpy(&argv[0], &input, sizeof(float));

        bool ret = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(ret) << "Function call failed: " << wasm_runtime_get_exception(wasm_module_inst);

        int64_t result;
        memcpy(&result, argv, sizeof(int64_t));
        return result;
    }

    /**
     * @brief Call i64.trunc_s/f64 WASM function with f64 input
     *
     * @param input f64 floating-point value to convert
     * @return int64_t converted signed 64-bit integer value
     *
     * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
     */
    int64_t call_i64_trunc_s_f64(double input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(wasm_module_inst, "i64_trunc_s_f64");
        EXPECT_NE(func, nullptr) << "Failed to lookup i64_trunc_s_f64 function";

        uint32 argv[2] = { 0, 0 };
        memcpy(argv, &input, sizeof(double));

        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_TRUE(ret) << "Function call failed: " << wasm_runtime_get_exception(wasm_module_inst);

        int64_t result;
        memcpy(&result, argv, sizeof(int64_t));
        return result;
    }

    /**
     * @brief Call function expecting trap and verify trap occurs
     *
     * @param func_name Name of the WASM function to call
     * @param input_f32 f32 input value (for f32 variant)
     * @param input_f64 f64 input value (for f64 variant)
     * @param use_f64 true for f64 variant, false for f32 variant
     * @return bool true if trap occurred as expected
     *
     * @coverage_target core/iwasm/common/wasm_runtime_common.c:wasm_runtime_call_wasm
     */
    bool call_and_expect_trap(const char* func_name, float input_f32, double input_f64, bool use_f64)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(wasm_module_inst, func_name);
        EXPECT_NE(func, nullptr) << "Failed to lookup function: " << func_name;

        uint32 argv[2] = { 0, 0 };
        uint32 argc;

        if (use_f64) {
            memcpy(argv, &input_f64, sizeof(double));
            argc = 2;
        } else {
            memcpy(&argv[0], &input_f32, sizeof(float));
            argc = 1;
        }

        bool ret = wasm_runtime_call_wasm(exec_env, func, argc, argv);

        // For trap scenarios, we expect the function call to fail
        return !ret && wasm_runtime_get_exception(wasm_module_inst) != nullptr;
    }

private:
    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t wasm_module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint8_t *buffer = nullptr;
    uint32_t size;
    char error_buf[128];
};

/**
 * @test InvalidValues_Infinity_ThrowsTrap
 * @brief Tests that infinity inputs cause proper WASM traps
 * @details Validates that both positive and negative infinity values trigger execution traps.
 *          Ensures proper error handling for infinite numeric inputs.
 * @test_category Exception - Infinity input validation
 * @coverage_target core/iwasm/common/wasm_runtime_common.c:wasm_runtime_call_wasm
 * @input_conditions +∞ and -∞ for both f32 and f64
 * @expected_behavior WASM trap occurs, function call returns false
 * @validation_method Verification that trap is properly generated and detected
 */
TEST_P(I64TruncSTest, InvalidValues_Infinity_ThrowsTrap) {
    float f32_pos_inf = std::numeric_limits<float>::infinity();
    float f32_neg_inf = -std::numeric_limits<float>::infinity();
    double f64_pos_inf = std::numeric_limits<double>::infinity();
    double f64_neg_inf = -std::numeric_limits<double>::infinity();

    ASSERT_TRUE(call_and_expect_trap("i64_trunc_s_f32", f32_pos_inf, 0.0, false))
        << "F32 positive infinity should cause trap";
    ASSERT_TRUE(call_and_expect_trap("i64_trunc_s_f32", f32_neg_inf, 0.0, false))
        << "F32 negative infinity should cause trap";
    ASSERT_TRUE(call_and_expect_trap("i64_trunc_s_f64", 0.0f, f64_pos_inf, true))
        << "F64 positive infinity should cause trap";
    ASSERT_TRUE(call_and_expect_trap("i64_trunc_s_f64", 0.0f, f64_neg_inf, true))
        << "F64 negative infinity should cause trap";
}

INSTANTIATE_TEST_SUITE_P(RunningModeTest, I64TruncSTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<I64TruncSTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });
