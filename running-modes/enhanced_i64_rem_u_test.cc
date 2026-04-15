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

static std::string WASM_FILE;
static const char *FUNC_NAME = "i64_rem_u_test";

class I64RemUTest : public testing::TestWithParam<RunningMode>
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

        WASM_FILE = "wasm-apps/i64_rem_u_test.wasm";

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

    uint64_t call_i64_rem_u(uint64_t dividend, uint64_t divisor)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, FUNC_NAME);
        EXPECT_NE(func, nullptr) << "Failed to lookup " << FUNC_NAME << " function";

        uint32_t argv[4];
        bool ret;

        // Pack i64 values into argv array (little endian)
        argv[0] = (uint32_t)(dividend & 0xFFFFFFFF);
        argv[1] = (uint32_t)(dividend >> 32);
        argv[2] = (uint32_t)(divisor & 0xFFFFFFFF);
        argv[3] = (uint32_t)(divisor >> 32);

        ret = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        EXPECT_TRUE(ret) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        // Extract i64 result from argv (little endian)
        uint64_t result = ((uint64_t)argv[1] << 32) | argv[0];
        return result;
    }

    bool call_i64_rem_u_expect_trap(uint64_t dividend, uint64_t divisor)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, FUNC_NAME);
        EXPECT_NE(func, nullptr) << "Failed to lookup " << FUNC_NAME << " function";

        uint32_t argv[4];

        // Pack i64 values into argv array (little endian)
        argv[0] = (uint32_t)(dividend & 0xFFFFFFFF);
        argv[1] = (uint32_t)(dividend >> 32);
        argv[2] = (uint32_t)(divisor & 0xFFFFFFFF);
        argv[3] = (uint32_t)(divisor >> 32);

        bool ret = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        return !ret; // Return true if trap occurred (call failed)
    }
};

INSTANTIATE_TEST_SUITE_P(RunningMode, I64RemUTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         testing::PrintToStringParamName());

/**
 * @test BasicRemainderOperation_ReturnsCorrectResult
 * @brief Validates i64.rem_u produces correct unsigned remainder results for typical inputs
 * @details Tests fundamental unsigned remainder operation with various 64-bit integer combinations.
 *          Verifies that i64.rem_u correctly computes a % b treating both operands as unsigned values.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i64_rem_u_operation
 * @input_conditions Standard u64 pairs: (10,3), (100,7), (1000000,17)
 * @expected_behavior Returns mathematical remainder: 1, 2, 14 respectively
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(I64RemUTest, BasicRemainderOperation_ReturnsCorrectResult)
{
    // Small positive numbers
    ASSERT_EQ(call_i64_rem_u(10, 3), 1ULL) << "10 % 3 failed";
    ASSERT_EQ(call_i64_rem_u(15, 4), 3ULL) << "15 % 4 failed";
    ASSERT_EQ(call_i64_rem_u(100, 7), 2ULL) << "100 % 7 failed";

    // Medium numbers
    ASSERT_EQ(call_i64_rem_u(1000, 13), 12ULL) << "1000 % 13 failed";
    ASSERT_EQ(call_i64_rem_u(1000000, 17), 9ULL) << "1000000 % 17 failed";

    // Larger numbers
    ASSERT_EQ(call_i64_rem_u(123456789ULL, 12345ULL), 6789ULL) << "Large number remainder failed";
}

/**
 * @test DivisionByZero_TriggersCorrectTrap
 * @brief Validates that division by zero triggers proper WASM trap behavior
 * @details Tests that any_number % 0 causes WASM_EXCEPTION_INTEGER_DIVIDE_BY_ZERO
 *          and proper trap handling in the WASM runtime for unsigned operations.
 * @test_category Error - Division by zero trap validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i64_rem_u_operation
 * @input_conditions Various unsigned dividends with zero divisor
 * @expected_behavior Runtime trap with integer division by zero exception
 * @validation_method Exception handling testing with trap detection
 */
TEST_P(I64RemUTest, DivisionByZero_TriggersCorrectTrap)
{
    // Test various dividends with zero divisor should all trap
    ASSERT_TRUE(call_i64_rem_u_expect_trap(1, 0)) << "1 % 0 should trap";
    ASSERT_TRUE(call_i64_rem_u_expect_trap(100, 0)) << "100 % 0 should trap";
    ASSERT_TRUE(call_i64_rem_u_expect_trap(1000000, 0)) << "1000000 % 0 should trap";

    // Large unsigned values with zero divisor
    ASSERT_TRUE(call_i64_rem_u_expect_trap(9223372036854775808ULL, 0)) << "Large unsigned % 0 should trap";
    ASSERT_TRUE(call_i64_rem_u_expect_trap(18446744073709551615ULL, 0)) << "UINT64_MAX % 0 should trap";

    // Zero dividend with zero divisor should also trap
    ASSERT_TRUE(call_i64_rem_u_expect_trap(0, 0)) << "0 % 0 should trap";
}
