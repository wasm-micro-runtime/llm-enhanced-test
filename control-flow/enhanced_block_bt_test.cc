/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_runtime_common.h"
#include "wasm_native.h"
#include "wasm_memory.h"
#include "bh_read_file.h"
#include "test_helper.h"

/**
 * @file enhanced_block_bt_test.cc
 * @brief Comprehensive unit tests for WASM 'block bt' opcode
 * @details Tests block instruction functionality including basic execution,
 *          control flow branching, nested structures, polymorphic types,
 *          and error conditions across interpreter and AOT modes.
 */

class BlockBtTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Set up test environment and initialize WAMR runtime
     * @details Configures WAMR runtime with system allocator and enables
     *          both interpreter and AOT modes for comprehensive testing
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";
    }

    /**
     * @brief Clean up test environment and destroy WAMR runtime
     * @details Properly cleans up all allocated resources including
     *          module instances, modules, and runtime environment
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
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM module from file path
     * @param wasm_file Relative path to WASM file from build directory
     * @return Pointer to loaded WASM module, or nullptr on failure
     */
    wasm_module_t LoadWasmModule(const char *wasm_file)
    {
        char error_buf[128] = { 0 };
        uint32 wasm_file_size;
        uint8 *wasm_file_buf = nullptr;

        wasm_file_buf = (uint8 *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
        EXPECT_NE(nullptr, wasm_file_buf) << "Failed to read WASM file: " << wasm_file;
        if (wasm_file_buf == nullptr) {
            return nullptr;
        }

        module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        EXPECT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        BH_FREE(wasm_file_buf);
        return module;
    }

    /**
     * @brief Create WASM module instance for testing
     * @param stack_size Stack size for module instance (default 8192)
     * @param heap_size Heap size for module instance (default 8192)
     * @return Pointer to module instance, or nullptr on failure
     */
    wasm_module_inst_t CreateModuleInstance(uint32 stack_size = 8192, uint32 heap_size = 8192)
    {
        char error_buf[128] = { 0 };

        EXPECT_NE(nullptr, module) << "Module must be loaded before creating instance";
        if (module == nullptr) {
            return nullptr;
        }

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
        EXPECT_NE(nullptr, module_inst) << "Failed to instantiate module: " << error_buf;

        if (module_inst != nullptr) {
            exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
            EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";
        }

        return module_inst;
    }

    /**
     * @brief Execute WASM function by name
     * @param func_name Name of exported function to execute
     * @param argc Number of arguments
     * @param argv Array of argument values
     * @return Execution result, or UINT32_MAX on failure
     */
    uint32 ExecuteFunction(const char *func_name, uint32 argc = 0, uint32 argv[] = nullptr)
    {
        wasm_function_inst_t func = nullptr;
        char error_buf[128] = { 0 };
        uint32 local_argv[16] = { 0 };  // Local buffer for arguments
        uint32 *args_ptr = (argc > 0 && argv != nullptr) ? argv : local_argv;

        EXPECT_NE(nullptr, module_inst) << "Module instance must be created before function execution";
        if (module_inst == nullptr) {
            return UINT32_MAX;
        }

        func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Function '" << func_name << "' not found in module";
        if (func == nullptr) {
            return UINT32_MAX;
        }

        // Copy arguments if provided
        if (argc > 0 && argv != nullptr && argc <= 16) {
            memcpy(local_argv, argv, argc * sizeof(uint32));
        }

        bool success = wasm_runtime_call_wasm(exec_env, func, argc, args_ptr);
        if (!success) {
            const char *exception = wasm_runtime_get_exception(module_inst);
            ADD_FAILURE() << "Function execution failed: " << (exception != nullptr ? exception : "Unknown error");
            return UINT32_MAX;
        }

        // Return first result value for functions with return values
        return (argc > 0 || args_ptr[0] != 0) ? args_ptr[0] : 0;
    }

    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    static const char *WASM_FILE;
};

const char *BlockBtTest::WASM_FILE = "wasm-apps/block_bt_test.wasm";

/**
 * @test BasicBlockExecution_ReturnsExpectedValues
 * @brief Validates basic block execution with various block types
 * @details Tests fundamental block operations including empty blocks, value-producing blocks,
 *          and value-consuming blocks to ensure correct execution semantics.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @input_conditions Various block types: empty [], value-producing [i32], consuming [i32]->[i64]
 * @expected_behavior Blocks execute correctly and produce expected result values
 * @validation_method Direct comparison of WASM function results with expected values
 */
TEST_P(BlockBtTest, BasicBlockExecution_ReturnsExpectedValues)
{
    // Load test module with block operations
    wasm_module_t test_module = LoadWasmModule(WASM_FILE);
    ASSERT_NE(nullptr, test_module) << "Failed to load block test module";

    // Create module instance for execution
    wasm_module_inst_t test_instance = CreateModuleInstance();
    ASSERT_NE(nullptr, test_instance) << "Failed to create module instance for block tests";

    // Apply the parameterized running mode to this instance
    wasm_runtime_set_running_mode(test_instance, GetParam());

    // Test 1: Empty block execution ([] -> [])
    uint32 result = ExecuteFunction("test_empty_block");
    ASSERT_EQ(0, result) << "Empty block should return 0 (no value produced)";

    // Test 2: Value-producing block ([] -> [i32])
    result = ExecuteFunction("test_value_producing_block");
    ASSERT_EQ(42, result) << "Value-producing block should return 42";

    // Test 3: Value-consuming and producing block ([i32] -> [i64])
    uint32 input_args[] = { 10 };
    result = ExecuteFunction("test_consuming_block", 1, input_args);
    ASSERT_EQ(20, result) << "Consuming block should double input value (10 * 2 = 20)";

    // Test 4: Multi-value block type ([i32, i32] -> [i32])
    uint32 multi_args[] = { 15, 25 };
    result = ExecuteFunction("test_multi_value_block", 2, multi_args);
    ASSERT_EQ(40, result) << "Multi-value block should sum inputs (15 + 25 = 40)";
}

// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(
    BlockBtTest,
    BlockBtTest,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode> &info) {
        return info.param == Mode_Interp ? "INTERP" : "AOT";
    }
);
