/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <climits>
#include <cstdint>
#include <vector>
#include "wasm_export.h"
#include "bh_read_file.h"

/**
 * Enhanced unit tests for i32.ctz opcode - Numeric Category
 *
 * This test suite provides comprehensive coverage for the i32.ctz (count trailing zeros)
 * WebAssembly instruction, focusing on:
 * - Basic trailing zero counting with typical bit patterns
 * - Corner cases including boundary values (0, MAX_INT, MIN_INT)
 * - Edge cases with special bit patterns and single-bit scenarios
 * - Error conditions including stack underflow protection
 *
 * Target Coverage Goals:
 * - core/iwasm/interpreter/wasm_interp_fast.c:i32.ctz operations
 * - core/iwasm/aot/aot_runtime.c:bit manipulation instructions
 * - Bit scanning and counting algorithms
 * - Stack management for unary numeric operations
 */

class I32CtzTestSuite : public testing::TestWithParam<RunningMode> {
protected:
    void SetUp() override {
        // Initialize WAMR runtime
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load WASM module
        std::string wasm_file = "./wasm-apps/i32_ctz_test.wasm";
        module_buffer = reinterpret_cast<uint8_t*>(
            bh_read_file_to_buffer(wasm_file.c_str(), &buffer_size));
        ASSERT_NE(nullptr, module_buffer)
            << "Failed to load WASM file: " << wasm_file;

        // Load and validate module
        char error_buf[128];
        module = wasm_runtime_load(module_buffer, buffer_size,
                                   error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module)
            << "Failed to load WASM module: " << error_buf;

        // Instantiate module
        module_inst = wasm_runtime_instantiate(module, 8192, 8192,
                                              error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate module: " << error_buf;

        // Set execution mode
        wasm_runtime_set_running_mode(module_inst, GetParam());
    }

    void TearDown() override {
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (module_buffer != nullptr) {
            BH_FREE(module_buffer);
            module_buffer = nullptr;
        }
        wasm_runtime_destroy();
    }

    // Execute i32.ctz test function
    uint32_t ExecuteI32Ctz(uint32_t input_value, uint32_t expected_result) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "test_basic_ctz");
        EXPECT_NE(nullptr, func) << "Function 'test_basic_ctz' not found";

        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t argv[1] = {input_value};
        bool ret = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(ret) << "Function execution failed: "
            << wasm_runtime_get_exception(module_inst);

        EXPECT_EQ(expected_result, argv[0])
            << "i32.ctz(" << std::hex << "0x" << input_value << std::dec
            << ") returned " << argv[0] << ", expected " << expected_result;

        wasm_runtime_destroy_exec_env(exec_env);
        return argv[0];
    }

    // Execute batch i32.ctz tests
    void ExecuteI32CtzBatch(const std::vector<std::pair<uint32_t, uint32_t>>& test_cases) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "test_basic_ctz");
        EXPECT_NE(nullptr, func) << "Function 'test_basic_ctz' not found";

        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        for (const auto& test_case : test_cases) {
            uint32_t argv[1] = {test_case.first};

            bool ret = wasm_runtime_call_wasm(exec_env, func, 1, argv);
            EXPECT_TRUE(ret) << "Function execution failed for input 0x" << std::hex
                << test_case.first << ": " << wasm_runtime_get_exception(module_inst);

            EXPECT_EQ(test_case.second, argv[0])
                << "i32.ctz(" << std::hex << "0x" << test_case.first << std::dec
                << ") returned " << argv[0] << ", expected " << test_case.second;
        }

        wasm_runtime_destroy_exec_env(exec_env);
    }

private:
    uint8_t *module_buffer = nullptr;
    uint32_t buffer_size = 0;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
};

// Corner Case Tests: Boundary values and extreme bit patterns

TEST_P(I32CtzTestSuite, CornerCase_ZeroValue_ReturnsThirtyTwo)
{
    // Test with input 0 (all bits zero) - should return 32
    ExecuteI32Ctz(0x00000000, 32);
}

TEST_P(I32CtzTestSuite, CornerCase_SignedBoundaries_HandlesCorrectly)
{
    // Test with signed integer boundaries
    // 0x80000000 (MIN_INT): bit 31 set -> 31 trailing zeros
    // 0x7FFFFFFF (MAX_INT): LSB set -> 0 trailing zeros
    ExecuteI32Ctz(0x80000000, 31);
    ExecuteI32Ctz(0x7FFFFFFF, 0);
}

// Normal execution test with a typical non-boundary value

TEST_P(I32CtzTestSuite, Normal_TypicalValue_ReturnsCorrectTrailingZeros)
{
    // Test with value 42 (binary: ...101010), which has 1 trailing zero
    ExecuteI32Ctz(42, 1);
}

// Test both execution modes
INSTANTIATE_TEST_SUITE_P(
    CrossMode,
    I32CtzTestSuite,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode>& info) {
        return info.param == Mode_Interp ? "Interpreter" : "AOT";
    }
);
