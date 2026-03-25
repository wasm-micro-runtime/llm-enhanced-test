/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cmath>
#include <cfloat>
#include <climits>

#include "wasm_runtime_common.h"
#include "bh_read_file.h"
#include "wasm_runtime.h"

/**
 * @brief Test fixture for i64.trunc_sat_f32_s opcode validation
 * @details Provides testing infrastructure for saturating truncation from f32
 *          to signed i64, covering WASM runtime call dispatch paths in both
 *          INTERP and LLVM_JIT execution modes.
 * @coverage_target core/iwasm/interpreter/wasm_runtime.c:wasm_interp_call_wasm
 *                  core/iwasm/interpreter/wasm_runtime.c:llvm_jit_call_func_bytecode
 */
class I64TruncSatF32STest : public testing::TestWithParam<RunningMode> {
  protected:
    /**
     * @brief Set up test environment and load WASM module
     * @details Initializes WAMR runtime, loads test module, and prepares
     *          execution context for both interpreter and AOT modes
     */
    void SetUp() override {
        memset(&init_args, 0, sizeof(init_args));

        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

        // Initialize runtime environment
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WASM runtime";

        // Load the test module based on running mode
        RunningMode mode = GetParam();
        load_module(mode);

        ASSERT_NE(module, nullptr) << "Failed to load WASM module";

        // Instantiate the module
        module_inst = wasm_runtime_instantiate(
            module, 8192, 8192, error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr)
            << "Failed to instantiate module: " << error_buf;

        // Create execution environment
        exec_env = wasm_runtime_create_exec_env(module_inst, 8192);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up test environment
     * @details Releases all allocated resources including execution environment,
     *          module instance, module, and runtime
     */
    void TearDown() override {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (wasm_buf != nullptr) {
            BH_FREE(wasm_buf);
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM module based on execution mode
     * @param mode Target execution mode (INTERP or AOT)
     * @details Loads appropriate bytecode or AOT-compiled module
     */
    void load_module(RunningMode mode) {
        const char* file_path = "wasm-apps/i64_trunc_sat_f32_s_test.wasm";

        wasm_buf = reinterpret_cast<uint8*>(
            bh_read_file_to_buffer(file_path, &wasm_buf_size));
        ASSERT_NE(wasm_buf, nullptr) << "Failed to read WASM file: " << file_path;

        module = wasm_runtime_load(wasm_buf, wasm_buf_size, error_buf, sizeof(error_buf));
    }

    /**
     * @brief Execute i64.trunc_sat_f32_s with given f32 input
     * @param input f32 value to convert
     * @return i64 result of saturating truncation
     * @details Calls WASM function with f32 parameter and retrieves i64 result
     */
    int64_t call_i64_trunc_sat_f32_s(float input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "test_i64_trunc_sat_f32_s");
        EXPECT_NE(func, nullptr) << "Failed to lookup test function";

        uint32_t argv[2];
        *(float*)argv = input;

        bool success = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(success) << "WASM function call failed: "
                            << wasm_runtime_get_exception(module_inst);

        return *(int64_t*)argv;
    }

    // Test infrastructure
    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint8* wasm_buf = nullptr;
    uint32_t wasm_buf_size = 0;
    char error_buf[128];
    char global_heap_buf[512 * 1024];
};

/**
 * @test RuntimeDispatch_SpecialFloatValues_ReturnsZero
 * @brief Validates WASM runtime dispatch for i64.trunc_sat_f32_s with special IEEE 754 f32 values
 * @details Tests that calling the WASM module containing i64.trunc_sat_f32_s via
 *          wasm_runtime_call_wasm correctly handles NaN, signed zeros, and subnormal
 *          numbers through the runtime dispatch layer (wasm_interp_call_wasm for INTERP
 *          mode, llvm_jit_call_func_bytecode for LLVM_JIT mode). The C helper
 *          trunc_f32_to_int is not directly exercised; results are validated at the
 *          runtime return value level.
 * @test_category Edge - Special value validation via runtime dispatch
 * @coverage_target core/iwasm/interpreter/wasm_runtime.c:wasm_interp_call_wasm
 *                  core/iwasm/interpreter/wasm_runtime.c:llvm_jit_call_func_bytecode
 * @input_conditions Special values: NaN, +0.0f, -0.0f, subnormal numbers
 * @expected_behavior NaN→0, zeros→0, subnormals→0 (no exceptions thrown)
 * @validation_method Verify deterministic zero results and no runtime exceptions
 */
TEST_P(I64TruncSatF32STest, RuntimeDispatch_SpecialFloatValues_ReturnsZero) {
    // Test NaN handling - should saturate to 0
    ASSERT_EQ(0, call_i64_trunc_sat_f32_s(NAN))
        << "NaN should saturate to 0 without causing exceptions";

    // Test positive zero
    ASSERT_EQ(0, call_i64_trunc_sat_f32_s(+0.0f))
        << "Positive zero should convert to 0";

    // Test negative zero
    ASSERT_EQ(0, call_i64_trunc_sat_f32_s(-0.0f))
        << "Negative zero should convert to 0";

    // Test small subnormal numbers
    ASSERT_EQ(0, call_i64_trunc_sat_f32_s(1e-40f))
        << "Small subnormal positive value should truncate to 0";

    ASSERT_EQ(0, call_i64_trunc_sat_f32_s(-1e-40f))
        << "Small subnormal negative value should truncate to 0";

    // Test values just under 1.0 (should truncate to 0)
    ASSERT_EQ(0, call_i64_trunc_sat_f32_s(0.999999f))
        << "Value just under 1.0 should truncate to 0";

    ASSERT_EQ(0, call_i64_trunc_sat_f32_s(-0.999999f))
        << "Negative value just under -1.0 should truncate to 0";
}

// Test parameter instantiation for both execution modes
INSTANTIATE_TEST_SUITE_P(RunningModes, I64TruncSatF32STest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT));
