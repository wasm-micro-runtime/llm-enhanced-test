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
 * @brief Test fixture for i64.trunc_sat_f64_u opcode validation
 * @details Provides comprehensive testing infrastructure for saturating truncation
 *          from f64 to unsigned i64, including module loading, execution context
 *          management, and cross-execution mode validation.
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:trunc_f64_to_uint
 */
class I64TruncSatF64UTest : public testing::TestWithParam<RunningMode> {
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

        // Set the running mode explicitly so the interpreter bytecode
        // dispatch loop (wasm_interp_call_func_bytecode) is entered in INTERP mode
        wasm_runtime_set_running_mode(module_inst, mode);

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
        const char* file_path = "wasm-apps/i64_trunc_sat_f64_u_test.wasm";

        wasm_buf = reinterpret_cast<uint8*>(
            bh_read_file_to_buffer(file_path, &wasm_buf_size));
        ASSERT_NE(wasm_buf, nullptr) << "Failed to read WASM file: " << file_path;

        module = wasm_runtime_load(wasm_buf, wasm_buf_size, error_buf, sizeof(error_buf));
    }

    /**
     * @brief Execute i64.trunc_sat_f64_u with given f64 input
     * @param input f64 value to convert
     * @return uint64_t result of saturating truncation
     * @details Calls WASM function with f64 parameter and retrieves u64 result
     */
    uint64_t call_i64_trunc_sat_f64_u(double input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "test_i64_trunc_sat_f64_u");
        EXPECT_NE(func, nullptr) << "Failed to lookup test function";

        uint32_t argv[4];
        *(double*)argv = input;

        bool success = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_TRUE(success) << "WASM function call failed: "
                            << wasm_runtime_get_exception(module_inst);

        return *(uint64_t*)argv;
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
 * @test BasicConversion_ReturnsCorrectUnsignedInteger
 * @brief Validates i64.trunc_sat_f64_u produces correct results for typical f64 inputs
 * @details Tests fundamental saturating truncation operation with positive values,
 *          zero, and fractional truncation. Verifies truncation toward zero behavior
 *          and proper handling of values within unsigned 64-bit range.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:trunc_f64_to_uint
 * @input_conditions Standard f64 values: 42.0, 1000.75, 0.0, large valid values
 * @expected_behavior Returns truncated u64: 42, 1000, 0, correct large values
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(I64TruncSatF64UTest, BasicConversion_ReturnsCorrectUnsignedInteger) {
    // Test positive value truncation
    ASSERT_EQ(42ULL, call_i64_trunc_sat_f64_u(42.0))
        << "Failed to truncate positive f64 value correctly";

    // Test fractional truncation toward zero
    ASSERT_EQ(1000ULL, call_i64_trunc_sat_f64_u(1000.75))
        << "Failed to truncate positive f64 fractional value";

    // Test zero conversion
    ASSERT_EQ(0ULL, call_i64_trunc_sat_f64_u(0.0))
        << "Failed to convert zero correctly";

    // Test positive zero conversion
    ASSERT_EQ(0ULL, call_i64_trunc_sat_f64_u(+0.0))
        << "Failed to convert positive zero correctly";

    // Test large positive value within u64 range
    ASSERT_EQ(999999999999ULL, call_i64_trunc_sat_f64_u(999999999999.9))
        << "Failed to truncate large positive f64 value";

    // Test fractional truncation properties
    ASSERT_EQ(3ULL, call_i64_trunc_sat_f64_u(3.7))
        << "Positive fractional should truncate toward zero";

    ASSERT_EQ(0ULL, call_i64_trunc_sat_f64_u(0.9))
        << "Value less than 1.0 should truncate to 0";

    ASSERT_EQ(1ULL, call_i64_trunc_sat_f64_u(1.9))
        << "Value between 1.0 and 2.0 should truncate to 1";
}

// Test parameter instantiation for both execution modes
INSTANTIATE_TEST_SUITE_P(RunningModes, I64TruncSatF64UTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT));
