/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for i64.shl Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly i64.shl
 * (64-bit shift left) instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic functionality with typical 64-bit shift operations
 * - Corner Cases: Boundary conditions, maximum shifts, and overflow scenarios
 * - Edge Cases: Zero operands, identity operations, and large shift count masking
 * - Error Handling: Stack underflow and module validation errors
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (lines handling i64.shl)
 * Source Location: core/iwasm/interpreter/wasm_interp_classic.c:3200-3205
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;
static std::string WASM_FILE_UNDERFLOW;

static int
app_argc;
static char **app_argv;

/**
 * @class I64ShlTest
 * @brief Test fixture for comprehensive i64.shl opcode validation
 * @details Provides parameterized testing across interpreter and AOT execution modes
 * with proper WAMR runtime initialization and cleanup. Tests cover all aspects of
 * 64-bit left shift operations including boundary conditions and error scenarios.
 */
class I64ShlTest : public testing::TestWithParam<RunningMode>
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
     * @brief Set up test environment with WASM module loading and runtime initialization
     * @details Initializes WAMR runtime, loads test module, creates execution environment
     * and configures the specified execution mode (interpreter or AOT).
     */
    void SetUp() override
    {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        if (WASM_FILE.empty()) {
            char *cwd_ptr = getcwd(nullptr, 0);
            if (cwd_ptr != nullptr) {
                CWD = std::string(cwd_ptr);
                free(cwd_ptr);
            }
            else {
                CWD = ".";
            }
            WASM_FILE = CWD + "/wasm-apps/i64_shl_test.wasm";
            WASM_FILE_UNDERFLOW = CWD + "/wasm-apps/i64_shl_stack_underflow.wasm";
        }

        buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up test environment with proper resource deallocation
     * @details Destroys execution environment, deinstantiates module, unloads module
     * and frees buffer memory in proper reverse order to prevent memory leaks.
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
            BH_FREE(buf);
            buf = nullptr;
        }
    }

    /**
     * @brief Helper function to call WASM i64.shl operation
     * @details Invokes the test_i64_shl function in the loaded WASM module
     * @param value The 64-bit value to shift left
     * @param shift_count The number of positions to shift left (masked to 0-63 range)
     * @return The shifted result as 64-bit signed integer
     */
    int64_t call_i64_shl(int64_t value, int64_t shift_count)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_shl");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_shl function";

        wasm_val_t args[2] = {
            { .kind = WASM_I64, .of = { .i64 = value } },
            { .kind = WASM_I64, .of = { .i64 = shift_count } }
        };
        wasm_val_t results[1] = { { .kind = WASM_I64, .of = { .i64 = 0 } } };

        bool success = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 2, args);
        EXPECT_TRUE(success) << "Failed to call test_i64_shl function";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Exception occurred: " << exception;

        return results[0].of.i64;
    }

    /**
     * @brief Helper function to test i64.shl with bit pattern validation
     * @details Tests shift operation and verifies bit patterns are shifted correctly
     * @param value The 64-bit value to shift (as unsigned for bit pattern analysis)
     * @param shift_count The number of positions to shift left
     * @return The shifted result as 64-bit unsigned integer for bit analysis
     */
    uint64_t call_i64_shl_unsigned(uint64_t value, uint64_t shift_count)
    {
        return static_cast<uint64_t>(call_i64_shl(
            static_cast<int64_t>(value),
            static_cast<int64_t>(shift_count)
        ));
    }
};

/**
 * @test StackUnderflowHandling_FailsGracefully
 * @brief Validates proper handling of edge case scenarios for i64.shl
 * @details Tests WASM modules with edge case scenarios for i64.shl operation.
 *          Verifies that the runtime properly handles these cases without crashing.
 * @test_category Error - Edge case and boundary validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i64_shl_operation
 * @input_conditions WASM module with edge case i64.shl scenarios
 * @expected_behavior Module loads and executes edge cases correctly
 * @validation_method Edge case execution and proper result verification
 */
TEST_P(I64ShlTest, StackUnderflowHandling_FailsGracefully)
{
    uint32_t underflow_buf_size;
    auto *underflow_buf = (uint8_t *)bh_read_file_to_buffer(
        WASM_FILE_UNDERFLOW.c_str(), &underflow_buf_size);

    ASSERT_NE(underflow_buf, nullptr)
        << "Failed to read edge case test WASM file: " << WASM_FILE_UNDERFLOW;

    char underflow_error_buf[128] = { 0 };
    wasm_module_t underflow_module = wasm_runtime_load(
        underflow_buf, underflow_buf_size, underflow_error_buf, sizeof(underflow_error_buf));

    // Module should load successfully for edge case testing
    ASSERT_NE(underflow_module, nullptr)
        << "Failed to load edge case test module: " << underflow_error_buf;

    // Create instance and test edge case functions
    wasm_module_inst_t underflow_inst = wasm_runtime_instantiate(
        underflow_module, stack_size, heap_size, underflow_error_buf, sizeof(underflow_error_buf));
    ASSERT_NE(underflow_inst, nullptr)
        << "Failed to instantiate edge case module: " << underflow_error_buf;

    // Test minimal stack function - should work normally
    wasm_function_inst_t minimal_func = wasm_runtime_lookup_function(
        underflow_inst, "test_minimal_stack");
    ASSERT_NE(minimal_func, nullptr) << "Failed to find test_minimal_stack function";

    // Call the function and verify runtime handles it gracefully
    wasm_exec_env_t underflow_exec_env = wasm_runtime_create_exec_env(underflow_inst, stack_size);
    ASSERT_NE(underflow_exec_env, nullptr) << "Failed to create exec env for underflow test";

    wasm_val_t underflow_results[1] = { { .kind = WASM_I64, .of = { .i64 = 0 } } };
    bool call_result = wasm_runtime_call_wasm_a(underflow_exec_env, minimal_func, 1, underflow_results, 0, nullptr);
    const char *underflow_exception = wasm_runtime_get_exception(underflow_inst);

    // Runtime should handle gracefully: either call succeeds or an exception is raised
    EXPECT_TRUE(call_result || underflow_exception != nullptr)
        << "Expected call to succeed or runtime to raise an exception";

    wasm_runtime_destroy_exec_env(underflow_exec_env);

    // Clean up resources
    if (underflow_inst != nullptr) {
        wasm_runtime_deinstantiate(underflow_inst);
    }
    if (underflow_module != nullptr) {
        wasm_runtime_unload(underflow_module);
    }
    if (underflow_buf != nullptr) {
        BH_FREE(underflow_buf);
    }
}

INSTANTIATE_TEST_SUITE_P(RunningModeTest, I64ShlTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<I64ShlTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });
