/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
static std::string WASM_FILE_STACK_UNDERFLOW;

static int
app_argc;
static char **app_argv;

class I64ExtendI32STest : public testing::TestWithParam<RunningMode>
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

    void SetUp() override
    {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        WASM_FILE = "wasm-apps/i64_extend_i32_s_test.wasm";
        WASM_FILE_STACK_UNDERFLOW = "wasm-apps/i64_extend_i32_s_stack_underflow.wasm";

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

    int64_t call_i64_extend_i32_s(int32_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "extend_i32_s_test");
        EXPECT_NE(func, nullptr) << "Failed to lookup extend_i32_s_test function";

        uint32_t argv[3] = { (uint32_t)input, 0, 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_EQ(ret, true) << "Function call failed";

        exception = wasm_runtime_get_exception(module_inst);
        if (exception != nullptr) {
            ADD_FAILURE() << "Runtime exception occurred: " << exception;
        }

        // Extract i64 result from argv (stored in first two elements)
        uint64_t result = ((uint64_t)argv[1] << 32) | argv[0];
        return (int64_t)result;
    }

    void test_stack_underflow(const std::string& wasm_file)
    {
        uint8_t *underflow_buf = nullptr;
        uint32_t underflow_buf_size;
        wasm_module_t underflow_module = nullptr;
        wasm_module_inst_t underflow_inst = nullptr;
        wasm_exec_env_t underflow_exec_env = nullptr;

        underflow_buf = (uint8_t *)bh_read_file_to_buffer(wasm_file.c_str(), &underflow_buf_size);
        ASSERT_NE(underflow_buf, nullptr) << "Failed to read underflow test WASM file";

        underflow_module = wasm_runtime_load(underflow_buf, underflow_buf_size,
                                           error_buf, sizeof(error_buf));

        // Test should succeed since our WAT file is valid
        if (underflow_module != nullptr) {
            underflow_inst = wasm_runtime_instantiate(underflow_module, stack_size, heap_size,
                                                    error_buf, sizeof(error_buf));

            if (underflow_inst != nullptr) {
                wasm_runtime_set_running_mode(underflow_inst, GetParam());
                underflow_exec_env = wasm_runtime_create_exec_env(underflow_inst, stack_size);

                if (underflow_exec_env != nullptr) {
                    wasm_function_inst_t func = wasm_runtime_lookup_function(underflow_inst, "stack_underflow_test");
                    if (func != nullptr) {
                        uint32_t argv[2] = { 0, 0 };
                        bool ret = wasm_runtime_call_wasm(underflow_exec_env, func, 0, argv);

                        ASSERT_EQ(ret, true) << "Stack underflow test function should execute successfully";
                    }
                    wasm_runtime_destroy_exec_env(underflow_exec_env);
                }
                wasm_runtime_deinstantiate(underflow_inst);
            }
            wasm_runtime_unload(underflow_module);
        }

        BH_FREE(underflow_buf);
    }
};

/**
 * @test BoundaryValues_MinMaxConstants_ReturnsCorrectConversion
 * @brief Validates i64.extend_i32_s handles INT32_MIN and INT32_MAX boundary values correctly
 * @details Tests sign extension behavior at the extreme boundaries of the i32 type range.
 *          Verifies correct bit patterns for maximum positive and minimum negative i32 values.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @input_conditions Boundary constants: INT32_MAX (0x7FFFFFFF), INT32_MIN (0x80000000)
 * @expected_behavior INT32_MAX→0x000000007FFFFFFF, INT32_MIN→0xFFFFFFFF80000000
 * @validation_method Exact bit pattern verification for boundary value sign extension
 */
TEST_P(I64ExtendI32STest, BoundaryValues_MinMaxConstants_ReturnsCorrectConversion)
{
    // Test INT32_MAX sign extension - positive value should have zero-filled high word
    int64_t result_max = call_i64_extend_i32_s(INT32_MAX);
    ASSERT_EQ(result_max, 0x000000007FFFFFFFLL)
        << "INT32_MAX sign extension failed: expected 0x000000007FFFFFFF, got 0x"
        << std::hex << result_max;

    // Test INT32_MIN sign extension - negative value should have one-filled high word
    int64_t result_min = call_i64_extend_i32_s(INT32_MIN);
    ASSERT_EQ(result_min, (int64_t)0xFFFFFFFF80000000LL)
        << "INT32_MIN sign extension failed: expected 0xFFFFFFFF80000000, got 0x"
        << std::hex << result_min;
}

INSTANTIATE_TEST_SUITE_P(RunningMode, I64ExtendI32STest,
                        testing::Values(Mode_Interp
#if WASM_ENABLE_JIT != 0
                                      , Mode_LLVM_JIT
#endif
                        ));
