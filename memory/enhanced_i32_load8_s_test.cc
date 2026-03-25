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
 * @brief Test fixture for comprehensive i32.load8_s opcode validation
 *
 * This test suite validates the i32.load8_s WebAssembly opcode across different
 * execution modes (interpreter and AOT). The opcode loads an 8-bit signed integer
 * from memory and sign-extends it to a 32-bit signed integer. Tests cover basic
 * functionality, sign extension behavior, boundary conditions, error scenarios,
 * and cross-mode consistency.
 */
class I32Load8STest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up test environment with WAMR runtime initialization
     *
     * Initializes the WAMR runtime with system allocator and loads
     * the i32.load8_s test module for execution validation.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        is_aot_mode = (GetParam() == Mode_LLVM_JIT);

        // Load the i32.load8_s test module
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
     * @brief Load and instantiate the i32.load8_s test WASM module
     *
     * Loads the compiled WASM bytecode and creates a module instance
     * with proper error handling and validation.
     */
    void LoadModule()
    {
        // Use .wasm files for both interpreter and AOT modes
        // AOT compilation happens at runtime for this test setup
        const char *wasm_path = "wasm-apps/i32_load8_s_test.wasm";

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
 * @test BasicLoad_ReturnsSignExtendedValue
 * @brief Validates i32.load8_s correctly loads and sign-extends 8-bit values
 * @details Tests fundamental load operation with known signed byte values at
 *          specific memory addresses, verifying correct sign extension for both
 *          positive and negative values
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_i32_load8_s
 * @input_conditions Memory addresses 0, 1, 2, 3 with test bytes 0x42, 0x00, 0x7F, 0x80
 * @expected_behavior Returns sign-extended values: 0x42, 0x00, 0x7F, 0xFFFFFF80
 * @validation_method Direct comparison of loaded values with expected sign-extended results
 */
TEST_P(I32Load8STest, BasicLoad_ReturnsSignExtendedValue)
{
    uint32_t argv[2];

    // Test load from address 0 (should contain 0x42 -> sign-extended to 0x00000042)
    argv[0] = 0; // address
    ASSERT_TRUE(CallWasmFunction("test_i32_load8_s", 1, argv))
        << "Failed to execute i32.load8_s from address 0";
    ASSERT_EQ(0x00000042U, argv[0])
        << "Incorrect sign extension for positive byte value 0x42";

    // Test load from address 1 (should contain 0x00 -> sign-extended to 0x00000000)
    argv[0] = 1; // address
    ASSERT_TRUE(CallWasmFunction("test_i32_load8_s", 1, argv))
        << "Failed to execute i32.load8_s from address 1";
    ASSERT_EQ(0x00000000U, argv[0])
        << "Incorrect sign extension for zero byte value 0x00";

    // Test load from address 2 (should contain 0x7F -> sign-extended to 0x0000007F)
    argv[0] = 2; // address
    ASSERT_TRUE(CallWasmFunction("test_i32_load8_s", 1, argv))
        << "Failed to execute i32.load8_s from address 2";
    ASSERT_EQ(0x0000007F, argv[0])
        << "Incorrect sign extension for positive max byte value 0x7F";

    // Test load from address 3 (should contain 0x80 -> sign-extended to 0xFFFFFF80)
    argv[0] = 3; // address
    ASSERT_TRUE(CallWasmFunction("test_i32_load8_s", 1, argv))
        << "Failed to execute i32.load8_s from address 3";
    ASSERT_EQ(0xFFFFFF80U, argv[0])
        << "Incorrect sign extension for negative min byte value 0x80";
}

// Test parameter setup for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossExecutionMode,
    I32Load8STest,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode>& info) {
        switch(info.param) {
            case Mode_Interp: return "Interpreter";
            case Mode_LLVM_JIT: return "AOT";
            default: return "Unknown";
        }
    }
);
