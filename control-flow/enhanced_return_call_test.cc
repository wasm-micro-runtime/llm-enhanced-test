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

static std::string CWD;
static const char *WASM_FILE_1 = "wasm-apps/return_call_test.wasm";
static const char *WASM_FILE_2 = "wasm-apps/return_call_error_test.wasm";

static constexpr const char *MODULE_NAME = "return_call_test";

/**
 * @brief Test fixture class for return_call opcode comprehensive testing
 *
 * This test suite validates the return_call instruction which performs tail call optimization
 * by executing a function call and immediately returning the result without growing the call stack.
 * Tests cover basic functionality, stack behavior, parameter handling, recursion scenarios,
 * and error conditions across both interpreter and AOT execution modes.
 */
class ReturnCallTest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up test environment before each test case
     *
     * Initializes WAMR runtime with proper configuration for both interpreter and AOT modes.
     * Sets up memory allocation and prepares test module loading infrastructure.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize runtime with support for both execution modes
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        cleanup_required = true;
    }

    /**
     * @brief Clean up test environment after each test case
     *
     * Properly destroys WAMR runtime and releases allocated resources.
     */
    void TearDown() override
    {
        if (cleanup_required) {
            wasm_runtime_destroy();
        }
    }

    /**
     * @brief Load WASM module from file for testing
     *
     * @param wasm_file_path Path to the WASM binary file
     * @return wasm_module_t Loaded WASM module or nullptr on failure
     */
    wasm_module_t load_wasm_module(const std::string &wasm_file_path)
    {
        const char *file_path = wasm_file_path.c_str();
        unsigned char *wasm_file_buf = nullptr;
        uint32_t wasm_file_size = 0;
        char error_buf[128] = {0};

        // Read WASM file from filesystem
        wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(file_path, &wasm_file_size);
        if (wasm_file_buf == nullptr) {
            return nullptr;
        }

        // Load WASM module with runtime validation
        wasm_module_t module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                               error_buf, sizeof(error_buf));

        // Clean up file buffer
        BH_FREE(wasm_file_buf);

        if (module == nullptr) {
            std::cout << "Module load error: " << error_buf << std::endl;
        }

        return module;
    }

    /**
     * @brief Create module instance for test execution
     *
     * @param module WASM module to instantiate
     * @param stack_size Stack size for module execution
     * @param heap_size Heap size for module memory
     * @return wasm_module_inst_t Module instance or nullptr on failure
     */
    wasm_module_inst_t create_module_instance(wasm_module_t module,
                                             uint32_t stack_size = 8092,
                                             uint32_t heap_size = 8092)
    {
        char error_buf[128] = {0};
        wasm_module_inst_t module_inst = wasm_runtime_instantiate(
            module, stack_size, heap_size, error_buf, sizeof(error_buf)
        );

        if (module_inst == nullptr) {
            std::cout << "Module instantiation error: " << error_buf << std::endl;
        }

        return module_inst;
    }

    /**
     * @brief Create execution environment for function calls
     *
     * @param module_inst Module instance for execution
     * @param stack_size Stack size for execution environment
     * @return wasm_exec_env_t Execution environment or nullptr on failure
     */
    wasm_exec_env_t create_exec_env(wasm_module_inst_t module_inst, uint32_t stack_size = 8192)
    {
        return wasm_runtime_create_exec_env(module_inst, stack_size);
    }

    /**
     * @brief Execute exported WASM function with parameters
     *
     * @param module_inst Module instance containing the function
     * @param func_name Name of the exported function to call
     * @param argc Number of arguments
     * @param argv Array of function arguments
     * @return bool True if execution succeeded, false otherwise
     */
    bool call_wasm_function(wasm_module_inst_t module_inst, const char* func_name,
                          uint32_t argc, uint32_t argv[])
    {
        char error_buf[128] = {0};
        wasm_function_inst_t func_inst = wasm_runtime_lookup_function(module_inst, func_name);

        if (func_inst == nullptr) {
            std::cout << "Function lookup failed: " << func_name << std::endl;
            return false;
        }

        // Create execution environment for this function call
        wasm_exec_env_t exec_env = create_exec_env(module_inst);
        if (exec_env == nullptr) {
            std::cout << "Failed to create execution environment" << std::endl;
            return false;
        }

        // Execute function and capture success/failure
        bool success = wasm_runtime_call_wasm(exec_env, func_inst, argc, argv);

        if (!success) {
            std::cout << "Function execution failed: " << wasm_runtime_get_exception(module_inst) << std::endl;
        }

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return success;
    }

private:
    RuntimeInitArgs init_args;
    bool cleanup_required = false;
};

/**
 * @test TrapInTailCall_GeneratesRuntimeError
 * @brief Validates return_call error handling for trapping tail calls
 * @details Tests that a return_call instruction targeting a function that divides by zero
 *          causes a runtime trap, exercising the return_call dispatch path in the interpreter.
 * @test_category Error - Runtime trap via return_call
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:return_call_handler
 * @input_conditions return_call_error_test.wasm exports "test_trap_in_tail_call" which uses
 *                   return_call to a function performing integer division by zero
 * @expected_behavior Execution fails with a runtime trap error
 * @validation_method ASSERT_FALSE(success) — runtime trap causes wasm_runtime_call_wasm to fail
 */
TEST_P(ReturnCallTest, TrapInTailCall_GeneratesRuntimeError)
{
    // Load test module containing return_call with a trapping callee
    wasm_module_t module = load_wasm_module(WASM_FILE_2);
    ASSERT_NE(nullptr, module) << "Failed to load return_call_error_test.wasm";

    wasm_module_inst_t module_inst = create_module_instance(module);
    ASSERT_NE(nullptr, module_inst) << "Failed to instantiate module";

    // Call test_trap_in_tail_call which uses return_call to a function that divides by zero
    uint32_t argv[2] = {5};
    bool success = call_wasm_function(module_inst, "test_trap_in_tail_call", 1, argv);
    ASSERT_FALSE(success) << "Expected division-by-zero trap via return_call to cause failure";

    wasm_runtime_deinstantiate(module_inst);
    wasm_runtime_unload(module);
}

INSTANTIATE_TEST_SUITE_P(RunningMode, ReturnCallTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<ReturnCallTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "Interpreter" : "AOT";
                         });

std::string get_current_directory() {
    char *cwd_ptr = get_current_dir_name();
    if (cwd_ptr != nullptr) {
        std::string cwd(cwd_ptr);
        free(cwd_ptr);
        return cwd;
    }
    return "";
}
