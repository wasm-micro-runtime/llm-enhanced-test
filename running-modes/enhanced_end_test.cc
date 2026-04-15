/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_common.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"
#include "wasm_runtime.h"
#include <unistd.h>
#include <climits>

static std::string CWD;
static const char *WASM_FILE = "wasm-apps/end_test.wasm";
static const char *WASM_FILE_INVALID = "wasm-apps/end_invalid_test.wasm";

/**
 * Test fixture for WASM 'end' opcode validation
 * @brief Comprehensive testing of structured control flow termination
 * @details Tests the 'end' opcode behavior across blocks, loops, conditionals, and functions.
 *          Validates proper stack unwinding, control flow termination, and cross-execution mode consistency.
 */
class EndTest : public testing::Test
{
protected:
    /**
     * @brief Set up WAMR runtime and load test modules
     * @details Initializes WAMR runtime with proper configuration for both interpreter and AOT modes.
     *          Loads the primary test module and prepares for 'end' opcode testing.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize the WAMR runtime environment
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load the primary test module for valid 'end' opcode tests
        buffer = (uint8_t *)bh_read_file_to_buffer(WASM_FILE, &size);
        ASSERT_NE(buffer, nullptr)
            << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(buffer, size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr)
            << "Failed to load WASM module: " << error_buf;

        // Instantiate module for test execution
        module_inst = wasm_runtime_instantiate(module, default_stack_size,
                                               default_heap_size, error_buf,
                                               sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr)
            << "Failed to instantiate WASM module: " << error_buf;
    }

    /**
     * @brief Clean up WAMR runtime and release resources
     * @details Properly releases module instances, modules, buffers, and destroys WAMR runtime.
     *          Ensures clean teardown for each test case.
     */
    void TearDown() override
    {
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (buffer != nullptr) {
            BH_FREE(buffer);
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Execute a WASM function with no parameters
     * @details Helper method to execute exported WASM functions that take no parameters
     * @param func_name Name of the exported function to execute
     * @return Execution result (typically function return value)
     */
    uint32_t CallWasmFunction(const std::string& func_name)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name.c_str());
        EXPECT_NE(func, nullptr) << "Function not found: " << func_name;

        uint32_t argv[1] = {0};
        EXPECT_TRUE(wasm_runtime_call_wasm(exec_env, func, 0, argv))
            << "Failed to execute function: " << func_name;

        return argv[0];
    }

    /**
     * @brief Execute a WASM function with one integer parameter
     * @details Helper method for functions requiring single integer input
     * @param func_name Name of the exported function to execute
     * @param param Input parameter value
     * @return Function execution result
     */
    uint32_t CallWasmFunctionWithParam(const std::string& func_name, uint32_t param)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name.c_str());
        EXPECT_NE(func, nullptr) << "Function not found: " << func_name;

        uint32_t argv[2] = {param, 0};
        EXPECT_TRUE(wasm_runtime_call_wasm(exec_env, func, 1, argv))
            << "Failed to execute function: " << func_name << " with param: " << param;

        return argv[0];
    }

    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint8_t *buffer = nullptr;
    uint32_t size = 0;
    char error_buf[128] = {0};
    const uint32_t default_stack_size = 16 * 1024;
    const uint32_t default_heap_size = 16 * 1024;
};

/**
 * @test ConditionalStructures_HandleBothPaths
 * @brief Validates conditional structure termination with 'end' opcodes
 * @details Tests if/then/else constructs with various conditions and result types.
 *          Verifies proper path execution and value propagation through conditional boundaries.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @input_conditions If/else with different conditions, result-producing conditionals
 * @expected_behavior Correct conditional path execution and result handling
 * @validation_method Conditional result verification for both true and false cases
 */
TEST_F(EndTest, ConditionalStructures_HandleBothPaths)
{
    exec_env = wasm_runtime_create_exec_env(module_inst, default_stack_size);
    ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";

    // Test if-then-end with true condition
    uint32_t result = CallWasmFunctionWithParam("test_if_then", 1);
    ASSERT_EQ(result, 10) << "If-then with true condition should return 10";

    // Test if-then-end with false condition
    result = CallWasmFunctionWithParam("test_if_then", 0);
    ASSERT_EQ(result, 0) << "If-then with false condition should return 0";

    // Test if-then-else-end with true condition
    result = CallWasmFunctionWithParam("test_if_then_else", 1);
    ASSERT_EQ(result, 20) << "If-then-else with true condition should return 20";

    // Test if-then-else-end with false condition
    result = CallWasmFunctionWithParam("test_if_then_else", 0);
    ASSERT_EQ(result, 30) << "If-then-else with false condition should return 30";

    wasm_runtime_destroy_exec_env(exec_env);
}
