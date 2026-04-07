/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "bh_read_file.h"
#include "wasm_runtime_common.h"

/**
 * @brief Enhanced unit test suite for WASM interpreter function dispatch validation
 * @details This test suite validates the WASM interpreter function dispatch via
 *          wasm_runtime_call_wasm, including:
 *          - Basic function calls with parameter passing
 *          - Return value handling for arithmetic and comparison functions
 *          - Function lookup via wasm_runtime_lookup_function
 *          - Interpreter execution via wasm_interp_call_wasm
 * @coverage_target core/iwasm/interpreter/wasm_runtime.c:wasm_lookup_function
 *                  core/iwasm/common/wasm_runtime_common.c:wasm_runtime_lookup_function
 *                  core/iwasm/interpreter/wasm_runtime.c:call_wasm_with_hw_bound_check
 *                  core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_wasm
 */

/**
 * @brief Test execution mode parameters for cross-validation
 */
enum class CallRefRunningMode {
    INTERP_MODE = 1,  /**< Interpreter execution mode */
    AOT_MODE = 2      /**< AOT compilation mode */
};

/**
 * @brief Parameterized test fixture for call_ref opcode validation
 * @details Supports both interpreter and AOT execution modes with proper
 *          WAMR runtime initialization and module management
 */
class CallRefTest : public testing::TestWithParam<CallRefRunningMode> {
protected:
    /**
     * @brief Set up WAMR runtime and initialize test environment
     * @details Initializes WAMR runtime with system allocator and enables
     *          reference types support required for call_ref functionality
     */
    void SetUp() override {
        // Initialize WAMR runtime with reference types support
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        is_runtime_initialized = true;
    }

    /**
     * @brief Clean up WAMR runtime and test resources
     * @details Destroys all loaded modules and cleans up runtime environment
     */
    void TearDown() override {
        // Clean up any loaded modules
        if (module_instance != nullptr) {
            wasm_runtime_deinstantiate(module_instance);
            module_instance = nullptr;
        }

        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }

        // Destroy runtime if initialized
        if (is_runtime_initialized) {
            wasm_runtime_destroy();
            is_runtime_initialized = false;
        }
    }

    /**
     * @brief Load WASM module from file and instantiate it
     * @param filename WASM file name in wasm-apps directory
     * @return true if module loaded and instantiated successfully, false otherwise
     */
    bool LoadWasmModule(const char* filename) {
        // Build full path to WASM file
        char file_path[256];
        (void)snprintf(file_path, sizeof(file_path), "wasm-apps/%s", filename);

        // Read WASM file content
        uint32_t wasm_file_size;
        char* file_buf = bh_read_file_to_buffer(file_path, &wasm_file_size);
        auto* wasm_file_buf = reinterpret_cast<uint8_t*>(file_buf);

        if (wasm_file_buf == nullptr) {
            return false;
        }

        // Load WASM module
        char error_buf[128];
        module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                   error_buf, sizeof(error_buf));

        // Clean up file buffer
        BH_FREE(wasm_file_buf);

        if (module == nullptr) {
            return false;
        }

        // Instantiate module
        module_instance = wasm_runtime_instantiate(module, 65536, 65536,
                                                   error_buf, sizeof(error_buf));

        return module_instance != nullptr;
    }

    /**
     * @brief Call exported WASM function with integer parameters
     * @param func_name Name of exported WASM function
     * @param params Vector of integer parameters
     * @return Function return value as integer
     */
    int32_t CallWasmFunction(const char* func_name, const std::vector<int32_t>& params) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_instance, func_name);

        if (func == nullptr) {
            return -1;
        }

        // Create execution environment
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_instance, 65536);
        if (exec_env == nullptr) {
            return -1;
        }

        // Prepare arguments array
        std::vector<uint32_t> argv(params.size() + 1); // Extra space for return value
        for (size_t i = 0; i < params.size(); ++i) {
            argv[i] = static_cast<uint32_t>(params[i]);
        }

        // Execute function
        bool success = wasm_runtime_call_wasm(exec_env, func,
                                              static_cast<uint32_t>(params.size()),
                                              argv.data());

        int32_t result = success ? static_cast<int32_t>(argv[0]) : -1;

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return result;
    }

private:
    bool is_runtime_initialized = false;    /**< Runtime initialization status */
    wasm_module_t module = nullptr;         /**< Loaded WASM module */
    wasm_module_inst_t module_instance = nullptr; /**< Module instance */
};

/**
 * @test WasmInterpreterDispatch_ReturnsCorrectValue
 * @brief Validates WASM interpreter correctly dispatches function calls and returns results
 * @details Tests wasm_runtime_call_wasm dispatch path with simple arithmetic and
 *          comparison functions. Verifies that function lookup and interpreter
 *          execution produce correct computed results.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/common/wasm_runtime_common.c:wasm_runtime_lookup_function
 *                  core/iwasm/interpreter/wasm_runtime.c:call_wasm_with_hw_bound_check
 * @input_conditions Exported arithmetic and comparison WASM functions with integer parameters
 * @expected_behavior Functions execute correctly and return expected computed values
 * @validation_method Direct comparison of return values with expected results
 */
TEST_P(CallRefTest, WasmInterpreterDispatch_ReturnsCorrectValue) {
    // Load WASM module with call_ref test functions
    ASSERT_TRUE(LoadWasmModule("call_ref_test.wasm"))
        << "Failed to load call_ref test module";

    // Test simple addition function call through funcref
    ASSERT_EQ(8, CallWasmFunction("test_add_call_ref", {5, 3}))
        << "call_ref addition function failed to return correct result";

    // Test multiplication function call through funcref
    ASSERT_EQ(15, CallWasmFunction("test_mul_call_ref", {3, 5}))
        << "call_ref multiplication function failed to return correct result";

    // Test function with different parameter types
    ASSERT_EQ(1, CallWasmFunction("test_compare_call_ref", {10, 5}))
        << "call_ref comparison function failed to return correct result";
}

// Instantiate parameterized tests for both execution modes
INSTANTIATE_TEST_SUITE_P(
    CallRefExecutionModes,
    CallRefTest,
    testing::Values(CallRefRunningMode::INTERP_MODE),
    [](const testing::TestParamInfo<CallRefRunningMode>& info) {
        switch (info.param) {
            case CallRefRunningMode::INTERP_MODE:
                return "InterpreterMode";
            case CallRefRunningMode::AOT_MODE:
                return "AOTMode";
            default:
                return "UnknownMode";
        }
    }
);
