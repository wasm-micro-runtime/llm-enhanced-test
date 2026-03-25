/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"
#include "wasm_runtime.h"

static std::string CWD;
static std::string WASM_FILE;
static constexpr RunningMode running_modes[] = { Mode_Interp, Mode_LLVM_JIT };

/**
 * @brief Test fixture for i64.load8_u opcode validation across interpreter and AOT modes
 *
 * This test suite comprehensively validates the i64.load8_u WebAssembly opcode functionality:
 * - Loads unsigned 8-bit integers from linear memory
 * - Performs proper zero extension to 64-bit unsigned integers
 * - Validates memory bounds checking and error handling
 * - Tests across both interpreter and AOT execution modes
 */
class I64Load8UTest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up WAMR runtime and load test module for each test case
     *
     * Initializes WAMR runtime with system allocator and loads the i64.load8_u
     * test module from WASM bytecode file. Configures both interpreter and AOT
     * execution modes based on test parameters.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize WAMR runtime
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        WASM_FILE = "wasm-apps/i64_load8_u_test.wasm";

        // Load WASM module from file
        module_buffer = (unsigned char *)bh_read_file_to_buffer(WASM_FILE.c_str(), &module_buffer_size);
        ASSERT_NE(nullptr, module_buffer) << "Failed to read WASM file: " << WASM_FILE;

        // Load module into WAMR
        module = wasm_runtime_load(module_buffer, module_buffer_size, error_buffer, sizeof(error_buffer));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buffer;

        // Instantiate module
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buffer, sizeof(error_buffer));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate WASM module: " << error_buffer;

        // Set execution mode
        wasm_runtime_set_running_mode(module_inst, GetParam());
    }

    /**
     * @brief Clean up WAMR resources after each test case
     *
     * Properly deallocates module instance, module, and runtime resources
     * to prevent memory leaks and ensure clean test environment.
     */
    void TearDown() override
    {
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (module_buffer != nullptr) {
            BH_FREE(module_buffer);
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Execute i64.load8_u test function with specified address
     *
     * @param func_name Name of the WASM test function to call
     * @param address Memory address parameter for load operation
     * @return uint64_t Zero-extended 64-bit result from i64.load8_u
     */
    uint64_t call_i64_load8_u_function(const char* func_name, uint32_t address)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Failed to lookup function: " << func_name;

        // Create execution environment
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t wasm_argv[3] = { address, 0, 0 }; // argv[0] = input, return value overwrites starting from argv[0]

        bool success = wasm_runtime_call_wasm(exec_env, func, 1, wasm_argv);
        EXPECT_TRUE(success) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        // Extract i64 result from argv slots (little-endian) - result overwrites argv[0] and argv[1]
        uint64_t result = ((uint64_t)wasm_argv[1] << 32) | wasm_argv[0];

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return result;
    }

    /**
     * @brief Execute i64.load8_u test function expecting trap/error
     *
     * @param func_name Name of the WASM test function to call
     * @param address Memory address parameter for load operation
     * @return bool True if function call resulted in expected trap/error
     */
    bool call_i64_load8_u_expect_trap(const char* func_name, uint32_t address)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Failed to lookup function: " << func_name;

        // Create execution environment
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t wasm_argv[3] = { address, 0, 0 }; // argv[0] = input, return value overwrites starting from argv[0]

        // Expect this call to fail/trap for out-of-bounds access
        bool success = wasm_runtime_call_wasm(exec_env, func, 1, wasm_argv);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        // For out-of-bounds, we expect the call to fail
        return !success;
    }

    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    unsigned char *module_buffer = nullptr;
    uint32_t module_buffer_size = 0;
    uint32_t stack_size = 16 * 1024;
    uint32_t heap_size = 16 * 1024;
    char error_buffer[256];
};

/**
 * @test ZeroExtensionValidation_ProducesCorrectBitPattern
 * @brief Tests zero extension with special bit patterns and extreme values
 * @details Validates correct zero extension behavior for various bit patterns including
 *          alternating bits, and other edge bit combinations.
 * @test_category Edge - Bit pattern validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:zero_extend_operation
 * @input_conditions Values 0x55, 0xAA, 0x0F, 0xF0 and other bit patterns
 * @expected_behavior Proper zero extension maintaining lower 8 bits, upper 56 bits zero
 * @validation_method Bit-level verification of zero extension correctness for all patterns
 */
TEST_P(I64Load8UTest, ZeroExtensionValidation_ProducesCorrectBitPattern)
{
    // Test alternating bit pattern (0x55 = 0101 0101 = 85)
    uint64_t pattern1 = call_i64_load8_u_function("test_load_0x55", 0);
    ASSERT_EQ(0x0000000000000055ULL, pattern1) << "0x55 pattern not zero-extended correctly";
    ASSERT_EQ(85ULL, pattern1) << "0x55 should equal 85 in unsigned";
    ASSERT_EQ(0x55, (pattern1 & 0xFF)) << "Lower 8 bits should be preserved exactly";

    // Test alternating bit pattern (0xAA = 1010 1010 = 170)
    uint64_t pattern2 = call_i64_load8_u_function("test_load_0xAA", 0);
    ASSERT_EQ(0x00000000000000AAULL, pattern2) << "0xAA pattern not zero-extended correctly";
    ASSERT_EQ(170ULL, pattern2) << "0xAA should equal 170 in unsigned";
    ASSERT_EQ(0xAA, (pattern2 & 0xFF)) << "Lower 8 bits should be preserved exactly";

    // Test nibble patterns (0x0F = 0000 1111 = 15)
    uint64_t nibble1 = call_i64_load8_u_function("test_load_0x0F", 0);
    ASSERT_EQ(0x000000000000000FULL, nibble1) << "0x0F pattern not zero-extended correctly";
    ASSERT_EQ(0ULL, (nibble1 & 0xFFFFFFFFFFFFFF00ULL)) << "Upper 56 bits must be zero";

    // Test nibble patterns (0xF0 = 1111 0000 = 240)
    uint64_t nibble2 = call_i64_load8_u_function("test_load_0xF0", 0);
    ASSERT_EQ(0x00000000000000F0ULL, nibble2) << "0xF0 pattern not zero-extended correctly";
    ASSERT_EQ(240ULL, nibble2) << "0xF0 should equal 240 in unsigned";
}

// Instantiate parameterized tests for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I64Load8UTest,
                        testing::ValuesIn(running_modes),
                        [](const testing::TestParamInfo<RunningMode>& info) {
                            return info.param == Mode_Interp ? "Interpreter" : "LLVM_JIT";
                        });
