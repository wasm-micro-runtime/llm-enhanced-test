/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include "wasm_runtime_common.h"
#include "bh_read_file.h"
#include "wasm_runtime.h"
#include "bh_platform.h"
#include "wasm_export.h"

#ifndef WASM_ENABLE_INTERP
#define WASM_ENABLE_INTERP 1
#endif
#ifndef WASM_ENABLE_AOT
#define WASM_ENABLE_AOT 1
#endif

// Use WAMR's built-in RunningMode enum from wasm_export.h
// Mode_Interp = 0, Mode_AOT = 1

/**
 * @class CallIndirectTest
 * @brief Test fixture class for comprehensive call_indirect opcode testing
 *
 * This test suite validates the WebAssembly call_indirect instruction across
 * different execution modes (interpreter and AOT), ensuring correct behavior
 * for dynamic function dispatch through function tables.
 *
 * Features tested:
 * - Basic indirect function calls with various signatures
 * - Table boundary validation and error handling
 * - Function signature type checking and validation
 * - Stack management during indirect calls
 * - Cross-execution mode consistency verification
 */
class CallIndirectTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Set up test environment before each test case
     *
     * Initializes WAMR runtime with appropriate configuration for the current
     * execution mode (interpreter or AOT). Configures memory allocation,
     * enables required features, and prepares the runtime environment.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        init_args.n_native_symbols = 0;
        init_args.native_module_name = nullptr;
        init_args.native_symbols = nullptr;

        // Initialize runtime with proper error handling
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime environment";

        running_mode = GetParam();

        // Configure mode-specific settings
        if (running_mode == Mode_LLVM_JIT) {
            // Ensure LLVM JIT compilation capabilities are available
            ASSERT_TRUE(wasm_runtime_is_running_mode_supported(Mode_LLVM_JIT))
                << "LLVM JIT mode not supported in current WAMR build";
        }
    }

    /**
     * @brief Clean up test environment after each test case
     *
     * Properly destroys WAMR runtime environment and releases all allocated
     * resources to prevent memory leaks between test cases.
     */
    void TearDown() override
    {
        wasm_runtime_destroy();
    }

    /**
     * @brief Load and instantiate WASM module for testing
     * @param wasm_file_path Path to the WASM binary file relative to test execution directory
     * @return Pointer to instantiated WASM module instance, or nullptr on failure
     *
     * Loads a WASM module from file, validates it, and creates a module instance
     * configured for the current execution mode. Handles both interpreter and
     * AOT mode instantiation with appropriate error reporting.
     */
    wasm_module_inst_t LoadTestModule(const char* wasm_file_path)
    {
        uint32 wasm_file_size;
        uint8 *wasm_file_buf = nullptr;
        wasm_module_t wasm_module = nullptr;
        wasm_module_inst_t module_inst = nullptr;
        char error_buf[128] = {0};

        // Read WASM file from filesystem
        wasm_file_buf = (uint8*)bh_read_file_to_buffer(wasm_file_path, &wasm_file_size);
        EXPECT_NE(nullptr, wasm_file_buf)
            << "Failed to read WASM file: " << wasm_file_path;

        if (wasm_file_buf == nullptr) {
            return nullptr;
        }

        // Load and validate WASM module
        wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        EXPECT_NE(nullptr, wasm_module)
            << "Failed to load WASM module: " << error_buf;

        if (wasm_module == nullptr) {
            BH_FREE(wasm_file_buf);
            return nullptr;
        }

        // Create module instance with sufficient stack and heap
        uint32 stack_size = 16 * 1024;  // 16KB stack
        uint32 heap_size = 16 * 1024;   // 16KB heap

        module_inst = wasm_runtime_instantiate(wasm_module, stack_size, heap_size, error_buf, sizeof(error_buf));
        EXPECT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        // Store references for cleanup
        current_module = wasm_module;
        current_module_inst = module_inst;
        current_file_buf = wasm_file_buf;

        return module_inst;
    }

    /**
     * @brief Execute exported WASM function with error handling
     * @param module_inst WASM module instance containing the function
     * @param function_name Name of the exported function to execute
     * @param argc Number of arguments to pass to the function
     * @param argv Array of arguments (uint32 values)
     * @return true if function executed successfully, false on error
     *
     * Executes a WASM function by name with the provided arguments. Handles
     * both successful execution and trapped/error conditions with appropriate
     * error reporting for debugging.
     */
    bool ExecuteFunction(wasm_module_inst_t module_inst, const char* function_name,
                        uint32 argc, uint32 argv[])
    {
        wasm_function_inst_t func_inst = nullptr;
        wasm_exec_env_t exec_env = nullptr;
        bool success = false;

        // Find exported function by name
        func_inst = wasm_runtime_lookup_function(module_inst, function_name);
        EXPECT_NE(nullptr, func_inst)
            << "Function not found: " << function_name;

        if (func_inst == nullptr) {
            return false;
        }

        // Create execution environment
        exec_env = wasm_runtime_create_exec_env(module_inst, 8192);  // 8KB exec stack
        EXPECT_NE(nullptr, exec_env)
            << "Failed to create execution environment";

        if (exec_env == nullptr) {
            return false;
        }

        // Execute function with exception handling
        success = wasm_runtime_call_wasm(exec_env, func_inst, argc, argv);

        if (!success) {
            // Capture exception information for debugging
            const char* exception = wasm_runtime_get_exception(module_inst);
            if (exception != nullptr) {
                // Don't add failure here - let the caller decide if failure is expected
                // This allows tests to properly validate trap conditions
            }
        }

        wasm_runtime_destroy_exec_env(exec_env);
        return success;
    }

    /**
     * @brief Clean up allocated resources from module loading
     *
     * Releases WASM module, module instance, and file buffer resources.
     * Called automatically during TearDown or can be called explicitly
     * for early resource cleanup.
     */
    void CleanupModule()
    {
        if (current_module_inst != nullptr) {
            wasm_runtime_deinstantiate(current_module_inst);
            current_module_inst = nullptr;
        }

        if (current_module != nullptr) {
            wasm_runtime_unload(current_module);
            current_module = nullptr;
        }

        if (current_file_buf != nullptr) {
            BH_FREE(current_file_buf);
            current_file_buf = nullptr;
        }
    }

    // Test fixture member variables
    RuntimeInitArgs init_args;
    RunningMode running_mode;

    // Resource management
    wasm_module_t current_module = nullptr;
    wasm_module_inst_t current_module_inst = nullptr;
    uint8* current_file_buf = nullptr;
};

/**
 * @test FunctionSignatureComplexity_HandlesMaximumComplexity
 * @brief Validates call_indirect with complex function signatures and mixed value types
 * @details Tests indirect function calls with complex signatures involving multiple parameters,
 *          multiple return values, and mixed WebAssembly value types (i32, i64, f32, f64).
 *          Ensures type system correctly handles complex function dispatch scenarios.
 * @test_category Edge - Complex signature validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:function_signature_validation
 * @input_conditions Functions with multiple parameters/returns using all WebAssembly value types
 * @expected_behavior Complex signatures work correctly through indirect dispatch
 * @validation_method Type-specific assertions for multi-value function results
 */
TEST_P(CallIndirectTest, FunctionSignatureComplexity_HandlesMaximumComplexity)
{
    // Load WASM module with complex signature functions
    wasm_module_inst_t module_inst = LoadTestModule("wasm-apps/call_indirect_test.wasm");
    ASSERT_NE(nullptr, module_inst) << "Failed to load call_indirect complex signature module";

    uint32 argv[6] = {0};

    // Test 1: Function with multiple parameters and single return
    argv[0] = 10;        // i32 parameter
    argv[1] = 0x40490FDB; // f64 parameter (3.14159 in IEEE 754)
    argv[2] = 0x40490FDB; // f64 parameter high bits
    argv[3] = 4;         // table index for complex function
    ASSERT_TRUE(ExecuteFunction(module_inst, "test_call_indirect_complex", 4, argv))
        << "Failed to execute call_indirect with complex signature";
    ASSERT_GT(argv[0], 0) << "Complex function should return positive result";

    // Test 2: Function with multiple return values
    argv[0] = 5;         // input parameter
    argv[1] = 5;         // table index for multi-return function
    ASSERT_TRUE(ExecuteFunction(module_inst, "test_call_indirect_multi_return", 2, argv))
        << "Failed to execute call_indirect with multiple return values";
    ASSERT_EQ(5, argv[0]) << "First return value should equal input parameter";
    ASSERT_EQ(25, argv[1]) << "Second return value should be square of input";

    CleanupModule();
}

// Parameterized test instantiation for interpreter mode
INSTANTIATE_TEST_SUITE_P(
    CallIndirectModeTest,
    CallIndirectTest,
    testing::Values(Mode_Interp),
    [](const testing::TestParamInfo<CallIndirectTest::ParamType>& info) {
        return "InterpreterMode";
    }
);
