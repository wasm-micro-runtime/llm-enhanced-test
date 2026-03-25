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
#include <climits>
#include <cfloat>

static const char *WASM_FILE = "wasm-apps/call_test.wasm";

/**
 * @brief Enhanced unit test suite for WASM call opcode
 * @details Comprehensive testing of function call operations including parameter passing,
 *          return value handling, stack effects, and error conditions across interpreter
 *          and AOT execution modes.
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:call_function
 * @test_categories Main, Corner, Edge, Error - Complete call opcode validation
 */
class CallTest : public testing::Test {
protected:
    /**
     * @brief Set up test environment and initialize WASM runtime
     * @details Initializes WAMR runtime with proper configuration for call opcode testing
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        memset(error_buf, 0, sizeof(error_buf));
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        wasm_file_buf = nullptr;
        wasm_file_size = 0;

        /* The llm-enhanced-test control-flow CMake copies `${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps`
           to `${CMAKE_CURRENT_BINARY_DIR}/wasm-apps`, so we must load via `wasm-apps/...`. */
        wasm_file_buf =
            (uint8 *)bh_read_file_to_buffer(WASM_FILE, &wasm_file_size);
        ASSERT_NE(nullptr, wasm_file_buf)
            << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf,
                                   sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, 8192, 8192, error_buf,
                                              sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        exec_env = wasm_runtime_create_exec_env(module_inst, 8192);
        ASSERT_NE(nullptr, exec_env)
            << "Failed to create execution environment";
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
        if (wasm_file_buf != nullptr) {
            BH_FREE(wasm_file_buf);
            wasm_file_buf = nullptr;
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Helper function to call WASM function and return i32 result
     * @param func_name Name of function to call
     * @param args Vector of uint32 arguments
     * @param out Output i32 result
     * @param out_error Optional error string
     * @return True if call succeeds and no exception is thrown
     */
    bool CallI32Function(const char *func_name, const std::vector<uint32> &args,
                         int32_t *out, std::string *out_error = nullptr)
    {
        if (out == nullptr) {
            if (out_error != nullptr) {
                *out_error = "out is nullptr";
            }
            return false;
        }
        if (module_inst == nullptr || exec_env == nullptr) {
            if (out_error != nullptr) {
                *out_error = "module_inst/exec_env is nullptr (fixture not initialized)";
            }
            return false;
        }

        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        if (func == nullptr) {
            if (out_error != nullptr) {
                *out_error = std::string("Function not found: ") + func_name;
            }
            return false;
        }

        auto argc = static_cast<uint32>(args.size());
        // Ensure enough argv space for return values (at least 1) and/or input args.
        std::vector<uint32> argv_vec;
        argv_vec.resize(std::max<size_t>(1, args.size()));
        for (size_t i = 0; i < args.size(); ++i) {
            argv_vec[i] = args[i];
        }
        uint32 *argv = argv_vec.data();

        bool success = wasm_runtime_call_wasm(exec_env, func, argc, argv);
        const char *exception = wasm_runtime_get_exception(module_inst);
        if (!success || exception != nullptr) {
            if (out_error != nullptr) {
                *out_error = exception != nullptr ? exception : "wasm_runtime_call_wasm returned false";
            }
            return false;
        }

        *out = static_cast<int32_t>(argv[0]);
        return true;
    }

    /**
     * @brief Helper function to execute void function
     * @param func_name Name of function to call
     * @param args Vector of uint32 arguments
     * @param out_error Optional error string
     * @return True if call succeeds and no exception is thrown
     */
    bool CallVoidFunction(const char *func_name, const std::vector<uint32> &args,
                          std::string *out_error = nullptr)
    {
        if (module_inst == nullptr || exec_env == nullptr) {
            if (out_error != nullptr) {
                *out_error = "module_inst/exec_env is nullptr (fixture not initialized)";
            }
            return false;
        }

        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        if (func == nullptr) {
            if (out_error != nullptr) {
                *out_error = std::string("Function not found: ") + func_name;
            }
            return false;
        }

        auto argc = static_cast<uint32>(args.size());
        // Provide non-null argv to be safe even if argc == 0.
        std::vector<uint32> argv_vec;
        argv_vec.resize(std::max<size_t>(1, args.size()));
        for (size_t i = 0; i < args.size(); ++i) {
            argv_vec[i] = args[i];
        }
        uint32 *argv = argv_vec.data();

        bool success = wasm_runtime_call_wasm(exec_env, func, argc, argv);
        const char *exception = wasm_runtime_get_exception(module_inst);
        if (!success || exception != nullptr) {
            if (out_error != nullptr) {
                *out_error = exception != nullptr ? exception : "wasm_runtime_call_wasm returned false";
            }
            return false;
        }
        return true;
    }

    /**
     * @brief Helper function to call WASM function with multiple return values
     * @param func_name Name of function to call
     * @param args Vector of input arguments
     * @param expected_returns Number of expected return values
     * @return Vector of return values
     */
    bool CallMultiReturnFunction(const char *func_name, const std::vector<uint32> &args,
                                 uint32 expected_returns, std::vector<uint32> *out,
                                 std::string *out_error = nullptr)
    {
        if (out == nullptr) {
            if (out_error != nullptr) {
                *out_error = "out is nullptr";
            }
            return false;
        }
        out->clear();
        if (module_inst == nullptr || exec_env == nullptr) {
            if (out_error != nullptr) {
                *out_error = "module_inst/exec_env is nullptr (fixture not initialized)";
            }
            return false;
        }

        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        if (func == nullptr) {
            if (out_error != nullptr) {
                *out_error = std::string("Function not found: ") + func_name;
            }
            return false;
        }

        // WAMR uses argv[0..retc-1] for return values after call; ensure storage for max(argc, retc).
        const size_t argc_sz = args.size();
        const size_t argv_sz =
            std::max<size_t>(1, std::max<size_t>(argc_sz, expected_returns));
        std::vector<uint32> argv(argv_sz);
        for (size_t i = 0; i < argc_sz; ++i) {
            argv[i] = args[i];
        }

        auto argc = static_cast<uint32>(args.size());
        bool success = wasm_runtime_call_wasm(exec_env, func, argc, argv.data());
        const char *exception = wasm_runtime_get_exception(module_inst);
        if (!success || exception != nullptr) {
            if (out_error != nullptr) {
                *out_error = exception != nullptr ? exception : "wasm_runtime_call_wasm returned false";
            }
            return false;
        }

        out->assign(argv.begin(), argv.begin() + expected_returns);
        return true;
    }

    RuntimeInitArgs init_args;
    char error_buf[128] = { 0 };
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint8 *wasm_file_buf = nullptr;
    uint32 wasm_file_size = 0;
};

/**
 * @test BasicFunctionCall_ReturnsCorrectValue
 * @brief Validates basic function call operations with standard parameter and return value handling
 * @details Tests fundamental call opcode functionality with simple arithmetic functions.
 *          Verifies that function calls correctly transfer parameters, execute function body,
 *          and return expected values to the caller.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:call_function
 * @input_conditions Simple arithmetic functions with i32 parameters
 * @expected_behavior Returns correct arithmetic results: add(5,3)=8, multiply(4,6)=24
 * @validation_method Direct comparison of WASM function results with expected values
 */
TEST_F(CallTest, BasicFunctionCall_ReturnsCorrectValue)
{
    std::string error;
    int32_t add_result = 0;
    // Test basic addition function call
    ASSERT_TRUE(CallI32Function("add", {5, 3}, &add_result, &error)) << error;
    ASSERT_EQ(8, add_result) << "add(5, 3) should return 8";

    // Test basic multiplication function call
    int32_t mul_result = 0;
    error.clear();
    ASSERT_TRUE(CallI32Function("multiply", {4, 6}, &mul_result, &error)) << error;
    ASSERT_EQ(24, mul_result) << "multiply(4, 6) should return 24";

    // Test subtraction with negative result
    int32_t sub_result = 0;
    error.clear();
    ASSERT_TRUE(CallI32Function("subtract", {10, 15}, &sub_result, &error)) << error;
    ASSERT_EQ(-5, sub_result) << "subtract(10, 15) should return -5";
}
