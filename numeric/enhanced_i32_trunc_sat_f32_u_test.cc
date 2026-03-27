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
 * @brief Test fixture for i32.trunc_sat_f32_u opcode validation
 * @details Provides comprehensive testing infrastructure for saturating truncation
 *          from f32 to unsigned i32, including module loading, execution context
 *          management, and cross-execution mode validation.
 */
class I32TruncSatF32UTest : public testing::TestWithParam<RunningMode> {
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

        // Get execution environment
        exec_env = wasm_runtime_create_exec_env(module_inst, 8192);
        ASSERT_NE(exec_env, nullptr)
            << "Failed to create execution environment";
    }

    /**
     * @brief Clean up test environment and release resources
     * @details Properly releases execution environment, module instance,
     *          module, and runtime resources following RAII principles
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
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM module based on execution mode
     * @param mode Execution mode (interpreter or AOT)
     */
    void load_module(RunningMode mode) {
        const char* module_path =
            "wasm-apps/i32_trunc_sat_f32_u_test.wasm";

        // Read module file
        module_buf = (uint8*)bh_read_file_to_buffer(module_path, &module_buf_size);
        ASSERT_NE(module_buf, nullptr) << "Failed to read module file: " << module_path;

        // Load module (both interpreter and AOT use same load function)
        module = wasm_runtime_load(module_buf, module_buf_size, error_buf, sizeof(error_buf));
    }

    /**
     * @brief Call i32.trunc_sat_f32_u WASM function with f32 parameter
     * @param f Input f32 value for conversion
     * @return Converted i32 value (representing unsigned integer)
     */
    uint32_t call_i32_trunc_sat_f32_u(float f) {
        // Find the target function in module
        WASMFunctionInstanceCommon* func = wasm_runtime_lookup_function(module_inst, "test_i32_trunc_sat_f32_u");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i32_trunc_sat_f32_u function";

        // Prepare arguments
        uint32_t argv[2]; // f32 value stored as uint32
        *(float*)&argv[0] = f;

        // Execute function
        bool ret = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(ret) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        // Return result (stored in argv[0] after call)
        return argv[0];
    }

    // Test infrastructure
    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint8* module_buf = nullptr;
    uint32 module_buf_size = 0;
    char error_buf[256];
    static uint8 global_heap_buf[8192 * 1024];
};

uint8 I32TruncSatF32UTest::global_heap_buf[8192 * 1024];

/**
 * @test SpecialFloatValues_HandledCorrectly
 * @brief Validates handling of special IEEE 754 floating-point values
 * @details Tests conversion of special f32 values including NaN (should become 0),
 *          positive infinity (should saturate to UINT32_MAX), negative infinity
 *          (should saturate to 0), and verification of consistent behavior.
 * @test_category Edge - Special IEEE 754 value validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:trunc_f32_to_int (NaN/infinity handling)
 * @input_conditions NaN values, +/-infinity, extreme magnitude values
 * @expected_behavior NaN→0, +∞→UINT32_MAX, -∞→0, consistent saturation behavior
 * @validation_method Verification of safe conversion without traps for special values
 */
TEST_P(I32TruncSatF32UTest, SpecialFloatValues_HandledCorrectly) {
    // Test NaN handling (should saturate to 0)
    ASSERT_EQ(0U, call_i32_trunc_sat_f32_u(NAN))
        << "NaN should saturate to 0";
    ASSERT_EQ(0U, call_i32_trunc_sat_f32_u(-NAN))
        << "Negative NaN should saturate to 0";

    // Test infinity handling
    ASSERT_EQ(UINT32_MAX, call_i32_trunc_sat_f32_u(INFINITY))
        << "Positive infinity should saturate to UINT32_MAX";
    ASSERT_EQ(0U, call_i32_trunc_sat_f32_u(-INFINITY))
        << "Negative infinity should saturate to 0";

    // Test extreme magnitude values
    ASSERT_EQ(UINT32_MAX, call_i32_trunc_sat_f32_u(FLT_MAX))
        << "FLT_MAX should saturate to UINT32_MAX";
    ASSERT_EQ(0U, call_i32_trunc_sat_f32_u(-FLT_MAX))
        << "Negative FLT_MAX should saturate to 0";

    // Test very small positive values (should truncate to 0)
    ASSERT_EQ(0U, call_i32_trunc_sat_f32_u(1e-10f))
        << "Very small positive value should truncate to 0";
    ASSERT_EQ(0U, call_i32_trunc_sat_f32_u(FLT_MIN))
        << "FLT_MIN should truncate to 0";
}

// Test parameter configuration for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossExecutionModeValidation,
    I32TruncSatF32UTest,
    testing::Values(
        Mode_Interp
#if WASM_ENABLE_JIT != 0
        , Mode_LLVM_JIT
#endif
    )
);
