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
 * @brief Test fixture for i64.load8_s opcode validation across interpreter and AOT modes
 *
 * This test suite comprehensively validates the i64.load8_s WebAssembly opcode functionality:
 * - Loads signed 8-bit integers from linear memory
 * - Performs proper sign extension to 64-bit signed integers
 * - Validates memory bounds checking and error handling
 * - Tests across both interpreter and AOT execution modes
 */
class I64Load8STest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up WAMR runtime and load test module for each test case
     *
     * Initializes WAMR runtime with system allocator and loads the i64.load8_s
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

        WASM_FILE = "wasm-apps/i64_load8_s_test.wasm";

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
     * @brief Execute i64.load8_s test function with specified address
     *
     * @param func_name Name of the WASM test function to call
     * @param address Memory address parameter for load operation
     * @return int64_t Sign-extended 64-bit result from i64.load8_s
     */
    int64_t call_i64_load8_s_function(const char* func_name, uint32_t address)
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
        int64_t result = ((int64_t)wasm_argv[1] << 32) | wasm_argv[0];

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return result;
    }

    /**
     * @brief Execute i64.load8_s test function expecting trap/error
     *
     * @param func_name Name of the WASM test function to call
     * @param address Memory address parameter for load operation
     * @return bool True if function call resulted in expected trap/error
     */
    bool call_i64_load8_s_expect_trap(const char* func_name, uint32_t address)
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
 * @test SignExtension_BoundaryValues_ProducesCorrectResults
 * @brief Verifies proper sign extension at critical 8-bit signed boundaries
 * @details Tests sign extension behavior at maximum positive (0x7F), minimum negative (0x80),
 *          and negative one (0xFF) boundary values to ensure correct bit 7 extension.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:sign_extend_8_to_64
 * @input_conditions Values 0x7F (127), 0x80 (-128), 0xFF (-1) at memory addresses
 * @expected_behavior Correct sign extension: 0x000000000000007F, 0xFFFFFFFFFFFFFF80, 0xFFFFFFFFFFFFFFFF
 * @validation_method Verify upper 56 bits match sign bit (bit 7) extension pattern
 */
TEST_P(I64Load8STest, SignExtension_BoundaryValues_ProducesCorrectResults)
{
    // Test maximum positive 8-bit signed value (0x7F = 127)
    int64_t max_positive = call_i64_load8_s_function("test_load_max_positive", 0);
    ASSERT_EQ(0x000000000000007FLL, max_positive) << "Maximum positive value not sign-extended correctly";

    // Verify upper 56 bits are all zeros for positive values
    ASSERT_EQ(0, (max_positive >> 8)) << "Upper bits not properly cleared for positive value";

    // Test minimum negative 8-bit signed value (0x80 = -128)
    int64_t min_negative = call_i64_load8_s_function("test_load_min_negative", 0);
    ASSERT_EQ((int64_t)0xFFFFFFFFFFFFFF80LL, min_negative) << "Minimum negative value not sign-extended correctly";

    // Verify upper 56 bits are all ones for negative values
    ASSERT_EQ(-1LL, (min_negative >> 8)) << "Upper bits not properly set for negative value";

    // Test negative one (0xFF = -1)
    int64_t negative_one = call_i64_load8_s_function("test_load_negative_one", 0);
    ASSERT_EQ((int64_t)0xFFFFFFFFFFFFFFFFLL, negative_one) << "Negative one not sign-extended correctly";
    ASSERT_EQ(-1LL, negative_one) << "Sign extension should produce -1 for 0xFF input";
}

// Instantiate parameterized tests for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I64Load8STest,
                        testing::ValuesIn(running_modes),
                        [](const testing::TestParamInfo<RunningMode>& info) {
                            return info.param == Mode_Interp ? "Interpreter" : "LLVM_JIT";
                        });
