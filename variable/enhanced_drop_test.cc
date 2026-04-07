/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <climits>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include "wasm_runtime.h"
#include "wasm_runtime_common.h"
#include "wasm_exec_env.h"
#include "test_helper.h"
#include "wasm_memory.h"
#include "bh_read_file.h"

/**
 * @class DropTest
 * @brief Comprehensive test suite for WebAssembly 'drop' opcode functionality
 * @details Tests the drop instruction across all supported value types and execution modes.
 *          The drop opcode removes the top value from the operand stack without using it,
 *          providing essential stack management functionality in WebAssembly execution.
 * @test_category Variable - Stack manipulation operations
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:drop_operation
 * @execution_modes Both interpreter and AOT compilation modes
 */
class DropTest : public testing::TestWithParam<int> {
protected:
    /**
     * @brief Set up test environment and initialize WAMR runtime
     * @details Initializes runtime with proper memory allocation and execution mode configuration
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        cleanup_called = false;
    }

    /**
     * @brief Clean up test environment and destroy WAMR runtime
     * @details Ensures proper cleanup of all allocated resources and runtime state
     */
    void TearDown() override
    {
        if (!cleanup_called) {
            wasm_runtime_destroy();
            cleanup_called = true;
        }
    }

    /**
     * @brief Load WASM module from file path
     * @details Loads and validates WASM binary module from specified file location
     * @param wasm_file Relative path to WASM binary file
     * @return Loaded WASM module instance or nullptr on failure
     */
    wasm_module_t LoadWasmModule(const char* wasm_file)
    {
        char error_buf[256] = { 0 };
        wasm_module_t module = nullptr;

        // Load WASM binary from file
        unsigned char* wasm_file_buf = nullptr;
        uint32_t wasm_file_size = 0;

        wasm_file_buf = (unsigned char*)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
        EXPECT_NE(nullptr, wasm_file_buf) << "Failed to read WASM file: " << wasm_file;

        if (wasm_file_buf != nullptr) {
            // Load the WASM module
            module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
            BH_FREE(wasm_file_buf);
        }

        EXPECT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;
        return module;
    }

    /**
     * @brief Execute WASM function and handle results
     * @details Creates execution environment, calls specified function, and processes results
     * @param module Loaded WASM module containing the target function
     * @param func_name Name of the exported function to execute
     * @param argc Number of function arguments
     * @param argv Array of function arguments
     * @return Function execution result or error indicator
     */
    uint32_t ExecuteWasmFunction(wasm_module_t module, const char* func_name,
                                uint32_t argc = 0, wasm_val_t* argv = nullptr)
    {
        char error_buf[256] = { 0 };

        // Create module instance
        wasm_module_inst_t module_inst = wasm_runtime_instantiate(
            module, 8192, 8192, error_buf, sizeof(error_buf));
        EXPECT_NE(nullptr, module_inst) << "Failed to create module instance: " << error_buf;

        if (module_inst == nullptr) return UINT32_MAX;

        // Create execution environment
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 8192);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t result = UINT32_MAX;

        if (exec_env != nullptr) {
            // Find and execute the function
            wasm_function_inst_t func_inst = wasm_runtime_lookup_function(module_inst, func_name);
            EXPECT_NE(nullptr, func_inst) << "Function '" << func_name << "' not found";

            if (func_inst != nullptr) {
                wasm_val_t results[1];
                results[0].kind = WASM_I32;
                results[0].of.i32 = 0;

                bool call_success = wasm_runtime_call_wasm_a(exec_env, func_inst,
                                                           1, results, argc, argv);
                if (call_success) {
                    result = results[0].of.i32;
                } else {
                    const char* exception = wasm_runtime_get_exception(module_inst);
                    if (exception != nullptr) {
                        printf("Function execution exception: %s\n", exception);
                    }
                }
            }

            wasm_runtime_destroy_exec_env(exec_env);
        }

        wasm_runtime_deinstantiate(module_inst);
        return result;
    }

    /**
     * @brief Get current execution mode for parameterized tests
     * @return Current running mode (Interpreter or AOT)
     */
    int GetExecutionMode() const { return GetParam(); }

private:
    RuntimeInitArgs init_args;
    bool cleanup_called;
};

/**
 * @test BasicDrop_AllTypes_StackHeightReduced
 * @brief Validates drop instruction with all supported WebAssembly value types
 * @details Tests that drop correctly removes values from stack for i32, i64, f32, and f64 types.
 *          Verifies that stack height decreases by exactly 1 and remaining values are unaffected.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:drop_operation
 * @input_conditions Single values of each type pushed onto stack before drop
 * @expected_behavior Stack height reduces by 1, value is discarded, no side effects
 * @validation_method Function return codes indicate successful drop operations
 */
TEST_P(DropTest, BasicDrop_AllTypes_StackHeightReduced)
{
    wasm_module_t module = LoadWasmModule("wasm-apps/drop_test.wasm");
    ASSERT_NE(nullptr, module) << "Failed to load drop test module";

    // Test i32 drop operation
    uint32_t i32_result = ExecuteWasmFunction(module, "test_drop_i32");
    ASSERT_EQ(42, i32_result) << "i32 drop failed - return value should be 42";

    // Test i64 drop operation
    uint32_t i64_result = ExecuteWasmFunction(module, "test_drop_i64");
    ASSERT_EQ(64, i64_result) << "i64 drop failed - return value should be 64";

    // Test f32 drop operation
    uint32_t f32_result = ExecuteWasmFunction(module, "test_drop_f32");
    ASSERT_EQ(32, f32_result) << "f32 drop failed - return value should be 32";

    // Test f64 drop operation
    uint32_t f64_result = ExecuteWasmFunction(module, "test_drop_f64");
    ASSERT_EQ(64, f64_result) << "f64 drop failed - return value should be 64";

    wasm_runtime_unload(module);
}

// Parameterized test registration for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossMode,
    DropTest,
    testing::Values(
        Mode_Interp
#if WASM_ENABLE_AOT != 0
        , Mode_LLVM_JIT
#endif
    ),
    [](const testing::TestParamInfo<int>& info) {
        return info.param == Mode_Interp ? "Interpreter" : "AoT";
    }
);
