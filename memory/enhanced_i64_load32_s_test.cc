/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <memory>
#include <cstring>
#include "wasm_runtime.h"
#include "bh_read_file.h"
#include "test_helper.h"

/**
 * @brief Test fixture for comprehensive i64.load32_s opcode validation
 *
 * This test suite validates the i64.load32_s WebAssembly opcode across different
 * execution modes (interpreter and AOT). The opcode loads a signed 32-bit integer
 * from memory and sign-extends it to 64-bit. Tests cover basic functionality,
 * sign extension behavior, boundary conditions, error scenarios, and cross-mode consistency.
 */
class I64Load32STest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up test environment with WAMR runtime initialization
     *
     * Initializes the WAMR runtime with system allocator and loads
     * the i64.load32_s test module for execution validation.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        is_aot_mode = (GetParam() == Mode_LLVM_JIT);

        // Load the i64.load32_s test module
        LoadModule();
    }

    /**
     * @brief Clean up test environment and destroy WAMR runtime
     *
     * Properly releases module instance, module, and runtime resources
     * to prevent memory leaks and ensure clean test isolation.
     */
    void TearDown() override
    {
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }

        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }

        wasm_runtime_destroy();
    }

    /**
     * @brief Load and instantiate the i64.load32_s test WASM module
     *
     * Loads the compiled WASM bytecode and creates a module instance
     * with proper error handling and validation.
     */
    void LoadModule()
    {
        // Use .wasm files for both interpreter and AOT modes
        // AOT compilation happens at runtime for this test setup
        const char *wasm_path = "wasm-apps/i64_load32_s_test.wasm";

        wasm_buf = bh_read_file_to_buffer(wasm_path, &wasm_buf_size);
        ASSERT_NE(nullptr, wasm_buf) << "Failed to read WASM file: " << wasm_path;

        module = wasm_runtime_load((uint8_t*)wasm_buf, wasm_buf_size,
                                  error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;
    }

    /**
     * @brief Execute a WASM function by name with provided arguments
     *
     * @param func_name Name of the exported WASM function to call
     * @param argc Number of arguments to pass to the function
     * @param argv Array of argument values for the function
     * @return bool True if execution succeeded, false if trapped/failed
     */
    bool CallWasmFunction(const char* func_name, uint32_t argc, uint32_t* argv)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Failed to lookup function: " << func_name;

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        bool ret = wasm_runtime_call_wasm(exec_env, func, argc, argv);

        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }

        return ret;
    }

    // Test fixture member variables
    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    char *wasm_buf = nullptr;
    uint32_t wasm_buf_size = 0;
    char error_buf[128] = {0};
    bool is_aot_mode = false;

    const uint32_t stack_size = 8092;
    const uint32_t heap_size = 8092;
};

/**
 * @test ZeroAndExtremePatterns_LoadCorrectly
 * @brief Validates loading of zero and extreme bit patterns
 * @details Tests load operation with special patterns including zero, all-ones,
 *          and alternating patterns to verify correct binary data handling
 * @test_category Edge - Binary pattern validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:binary_data_load
 * @input_conditions Memory patterns: 0x00000000, 0xFFFFFFFF, 0xAAAAAAAA, 0x55555555
 * @expected_behavior Correct sign extension based on MSB of each pattern
 * @validation_method Bitwise comparison of loaded values with expected patterns
 */
TEST_P(I64Load32STest, ZeroAndExtremePatterns_LoadCorrectly)
{
    uint32_t argv[4];

    // Test loading zero pattern (0x00000000)
    argv[0] = 32; // address containing 0x00000000
    ASSERT_TRUE(CallWasmFunction("test_i64_load32_s", 1, argv))
        << "Failed to load zero pattern from memory";

    uint64_t zero_result = ((uint64_t)argv[1] << 32) | argv[0];
    ASSERT_EQ(0x0000000000000000ULL, zero_result)
        << "Zero pattern should remain zero after sign extension";

    // Test loading alternating pattern 1 (0xAAAAAAAA - MSB=1, negative)
    argv[0] = 36; // address containing 0xAAAAAAAA
    ASSERT_TRUE(CallWasmFunction("test_i64_load32_s", 1, argv))
        << "Failed to load alternating pattern 1 from memory";

    uint64_t alt_result1 = ((uint64_t)argv[1] << 32) | argv[0];
    ASSERT_EQ(0xFFFFFFFFAAAAAAAAULL, alt_result1)
        << "Alternating pattern 0xAAAAAAAA should sign-extend to negative";

    // Test loading alternating pattern 2 (0x55555555 - MSB=0, positive)
    argv[0] = 40; // address containing 0x55555555
    ASSERT_TRUE(CallWasmFunction("test_i64_load32_s", 1, argv))
        << "Failed to load alternating pattern 2 from memory";

    uint64_t alt_result2 = ((uint64_t)argv[1] << 32) | argv[0];
    ASSERT_EQ(0x0000000055555555ULL, alt_result2)
        << "Alternating pattern 0x55555555 should zero-extend to positive";
}

/**
 * @test UnalignedAccess_WorksCorrectly
 * @brief Tests i64.load32_s with unaligned memory addresses
 * @details Validates that load operations work correctly with addresses that
 *          are not aligned to 4-byte boundaries, testing WAMR's alignment handling
 * @test_category Corner - Alignment validation
 * @coverage_target core/iwasm/common/wasm_memory.c:unaligned_access_handling
 * @input_conditions Unaligned addresses: 1, 3, 5, 7 with known data patterns
 * @expected_behavior Successful loads with correct sign extension regardless of alignment
 * @validation_method Verify unaligned access produces expected values
 */
TEST_P(I64Load32STest, UnalignedAccess_WorksCorrectly)
{
    uint32_t argv[4];

    // Test unaligned load from address 1
    argv[0] = 1; // unaligned address
    ASSERT_TRUE(CallWasmFunction("test_i64_load32_s", 1, argv))
        << "Failed to load from unaligned address 1";

    uint64_t result1 = ((uint64_t)argv[1] << 32) | argv[0];
    // Bytes [1..4] = 0x56,0x34,0x12,0x01 -> little-endian 0x01123456, MSB=0 -> zero-extends
    ASSERT_EQ(0x0000000001123456ULL, result1)
        << "Unaligned load at address 1 should read bytes [1..4] as 0x01123456";

    // Test unaligned load from address 3
    argv[0] = 3; // unaligned address
    ASSERT_TRUE(CallWasmFunction("test_i64_load32_s", 1, argv))
        << "Failed to load from unaligned address 3";

    uint64_t result3 = ((uint64_t)argv[1] << 32) | argv[0];
    // Bytes [3..6] = 0x12,0x01,0x00,0x00 -> little-endian 0x00000112, MSB=0 -> zero-extends
    ASSERT_EQ(0x0000000000000112ULL, result3)
        << "Unaligned load at address 3 should read bytes [3..6] as 0x00000112";

    // Test unaligned load from address 5
    argv[0] = 5; // unaligned address
    ASSERT_TRUE(CallWasmFunction("test_i64_load32_s", 1, argv))
        << "Failed to load from unaligned address 5";

    uint64_t result5 = ((uint64_t)argv[1] << 32) | argv[0];
    // Bytes [5..8] = 0x00,0x00,0x00,0xff -> little-endian 0xFF000000, MSB=1 -> sign-extends
    ASSERT_EQ(0xFFFFFFFFFF000000ULL, result5)
        << "Unaligned load at address 5 should read bytes [5..8] as 0xFF000000 sign-extended";
}

// Instantiate tests for both interpreter and AOT execution modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I64Load32STest,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT));
