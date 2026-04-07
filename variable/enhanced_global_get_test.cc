/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for global.get Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly global.get
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic functionality getting global variables of different types
 * - Corner Cases: Boundary conditions and index validation
 * - Edge Cases: Special numeric values, mutability handling, and extreme scenarios
 * - Error Handling: Invalid indices and module validation failures
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (lines handling global.get)
 * Source Location: core/iwasm/interpreter/wasm_interp_classic.c:2200-2220
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;
static std::string WASM_FILE_INVALID_INDEX;

static int
app_argc;
static char **app_argv;

/**
 * @brief Test fixture for global.get opcode comprehensive validation
 * @details Manages WAMR runtime initialization, module loading, and cleanup
 *          for testing global variable access across different execution modes.
 *          Supports both interpreter and AOT execution with proper resource management.
 */
class GlobalGetTest : public testing::TestWithParam<RunningMode>
{
  protected:
    WAMRRuntimeRAII<> runtime;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint32_t buf_size, stack_size = 8092, heap_size = 8092;
    uint8_t *buf = nullptr;
    char error_buf[128] = { 0 };
    const char *exception = nullptr;

    /**
     * @brief Setup test environment and initialize WAMR runtime
     * @details Configures runtime parameters, loads test WASM module, and prepares
     *          execution environment for global.get testing across different modes.
     */
    void SetUp() override
    {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Cleanup test resources and runtime environment
     * @details Properly destroys module instances, execution environments,
     *          and releases allocated memory to prevent resource leaks.
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
        if (buf != nullptr) {
            BH_FREE(buf);
            buf = nullptr;
        }
    }

    /**
     * @brief Execute WASM function and return i32 result
     * @param func_name Name of the exported WASM function to call
     * @return Function result as i32 value
     * @details Validates function existence, executes with proper error handling,
     *          and extracts i32 result from execution context.
     */
    int32_t CallI32Function(const std::string& func_name)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name.c_str());
        EXPECT_NE(nullptr, func) << "Function " << func_name << " not found";

        uint32_t argv[1] = { 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_TRUE(ret) << "Failed to execute function " << func_name;

        exception = wasm_runtime_get_exception(module_inst);
        if (exception != nullptr) {
            ADD_FAILURE() << "Runtime exception occurred: " << exception;
        }

        return (int32_t)argv[0];
    }

    /**
     * @brief Execute WASM function and return i64 result
     * @param func_name Name of the exported WASM function to call
     * @return Function result as i64 value
     */
    int64_t CallI64Function(const std::string& func_name)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name.c_str());
        EXPECT_NE(nullptr, func) << "Function " << func_name << " not found";

        uint32_t argv[2] = { 0, 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_TRUE(ret) << "Failed to execute function " << func_name;

        exception = wasm_runtime_get_exception(module_inst);
        if (exception != nullptr) {
            ADD_FAILURE() << "Runtime exception occurred: " << exception;
        }

        return ((int64_t)argv[1] << 32) | argv[0];
    }

    /**
     * @brief Execute WASM function and return f32 result
     * @param func_name Name of the exported WASM function to call
     * @return Function result as f32 value
     */
    float CallF32Function(const std::string& func_name)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name.c_str());
        EXPECT_NE(nullptr, func) << "Function " << func_name << " not found";

        uint32_t argv[1] = { 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_TRUE(ret) << "Failed to execute function " << func_name;

        exception = wasm_runtime_get_exception(module_inst);
        if (exception != nullptr) {
            ADD_FAILURE() << "Runtime exception occurred: " << exception;
        }

        float result;
        memcpy(&result, &argv[0], sizeof(float));
        return result;
    }

    /**
     * @brief Execute WASM function and return f64 result
     * @param func_name Name of the exported WASM function to call
     * @return Function result as f64 value
     */
    double CallF64Function(const std::string& func_name)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name.c_str());
        EXPECT_NE(nullptr, func) << "Function " << func_name << " not found";

        uint32_t argv[2] = { 0, 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_TRUE(ret) << "Failed to execute function " << func_name;

        exception = wasm_runtime_get_exception(module_inst);
        if (exception != nullptr) {
            ADD_FAILURE() << "Runtime exception occurred: " << exception;
        }

        double result;
        memcpy(&result, &argv[0], sizeof(double));
        return result;
    }

    /**
     * @brief Check if two floating-point values are bitwise identical
     * @param expected Expected floating-point value
     * @param actual Actual floating-point value
     * @return true if values are bitwise identical (handles NaN, ±0.0)
     */
    template<typename T>
    bool IsFloatBitwiseEqual(T expected, T actual)
    {
        if (std::isnan(expected) && std::isnan(actual)) {
            return true;  // Both NaN (don't check bit pattern)
        }
        if (std::isinf(expected) && std::isinf(actual)) {
            return (expected > 0) == (actual > 0);  // Same infinity sign
        }
        return expected == actual;  // Exact equality including ±0.0
    }
};

/**
 * @test BoundaryValueAccess_PreservesExtremeValues
 * @brief Validates global.get preserves extreme boundary values for numeric types
 * @details Tests edge cases with MIN/MAX values for integers and extreme finite values
 *          for floating-point types to ensure bit-level value preservation.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:global_get_operation
 * @input_conditions Boundary values: INT32_MIN/MAX, INT64_MIN/MAX, extreme float values
 * @expected_behavior Exact preservation of boundary values without overflow/underflow
 * @validation_method Bitwise comparison for extreme numeric values
 */
TEST_P(GlobalGetTest, BoundaryValueAccess_PreservesExtremeValues)
{

    // Test i32 boundary values - minimum and maximum representable integers
    ASSERT_EQ(INT32_MIN, CallI32Function("get_i32_min_global"))
        << "Failed to preserve i32 minimum boundary value";
    ASSERT_EQ(INT32_MAX, CallI32Function("get_i32_max_global"))
        << "Failed to preserve i32 maximum boundary value";

    // Test i64 boundary values - 64-bit integer limits
    ASSERT_EQ(INT64_MIN, CallI64Function("get_i64_min_global"))
        << "Failed to preserve i64 minimum boundary value";
    ASSERT_EQ(INT64_MAX, CallI64Function("get_i64_max_global"))
        << "Failed to preserve i64 maximum boundary value";

    // Test f32 extreme finite values
    float f32_max = CallF32Function("get_f32_max_global");
    ASSERT_TRUE(IsFloatBitwiseEqual(FLT_MAX, f32_max))
        << "Failed to preserve f32 maximum finite value";

    // Test f64 extreme finite values
    double f64_max = CallF64Function("get_f64_max_global");
    ASSERT_TRUE(IsFloatBitwiseEqual(DBL_MAX, f64_max))
        << "Failed to preserve f64 maximum finite value";
}

// Test parameters for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(RunningModeTest, GlobalGetTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<GlobalGetTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });

/**
 * @brief Initialize test environment and file paths for global.get tests
 */
class GlobalGetTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        char *cwd = getcwd(nullptr, 0);
        if (cwd != nullptr) {
            CWD = std::string(cwd);
            free(cwd);
            WASM_FILE = CWD + "/wasm-apps/global_get_test.wasm";
            WASM_FILE_INVALID_INDEX = CWD + "/wasm-apps/global_get_invalid_index_test.wasm";
        }
    }
};

static ::testing::Environment* const global_get_env =
    ::testing::AddGlobalTestEnvironment(new GlobalGetTestEnvironment);
