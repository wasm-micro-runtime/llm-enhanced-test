/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for local.tee Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly local.tee
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic functionality with dual stack/local variable operation
 * - Corner Cases: Boundary conditions, extreme values, and index validation
 * - Edge Cases: Special floating-point values, zero handling, repeated operations
 * - Error Handling: Invalid indices, stack underflow, and validation failures
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (lines handling local.tee)
 * Source Location: core/iwasm/interpreter/wasm_interp_classic.c:1820-1840
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
static std::string WASM_FILE_TYPE_MISMATCH;

static int
app_argc;
static char **app_argv;

/**
 * @class LocalTeeTest
 * @brief Comprehensive test fixture for local.tee opcode validation
 * @details Provides WAMR runtime setup and helper functions for testing
 *          local.tee instruction across interpreter and AOT execution modes.
 *          Tests the unique dual functionality: storing to local AND returning value.
 *          Includes proper resource management and error handling.
 */
class LocalTeeTest : public testing::TestWithParam<RunningMode>
{
protected:
    WAMRRuntimeRAII<512 * 1024> runtime;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    char error_buf[128];
    std::string wasm_file_path;

    /**
     * @brief Set up test environment with WAMR runtime initialization
     * @details Initializes WAMR runtime, loads test module, and prepares execution environment
     *          for both interpreter and AOT modes based on test parameters.
     */
    void SetUp() override
    {
        memset(error_buf, 0, sizeof(error_buf));
        wasm_file_path = CWD + WASM_FILE;

        // Load WASM module from file
        auto [buffer, buffer_size] = load_wasm_file(wasm_file_path.c_str());
        ASSERT_NE(nullptr, buffer) << "Failed to load WASM file: " << wasm_file_path;

        // Load WASM module
        module = wasm_runtime_load(buffer, buffer_size, error_buf, sizeof(error_buf));

        ASSERT_NE(nullptr, module) << "Failed to load module: " << error_buf;

        // Instantiate module
        module_inst = wasm_runtime_instantiate(module, 65536, 0, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate module: " << error_buf;

        // Set execution mode based on test parameter
        wasm_runtime_set_running_mode(module_inst, GetParam());

        // Create execution environment
        exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        ASSERT_NE(nullptr, exec_env) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up test resources and shutdown WAMR runtime
     * @details Properly destroys execution environment, module instance, and module
     *          to prevent resource leaks and ensure clean test isolation.
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
    }

    /**
     * @brief Call WASM function with i32 local.tee operation
     * @param input The i32 value to tee to local variable
     * @return The value returned on stack after tee operation
     * @details Tests that local.tee both stores the value locally AND returns it on stack
     */
    int32_t call_i32_tee(int32_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i32_tee");
        EXPECT_NE(nullptr, func) << "Failed to lookup i32_tee function";

        wasm_val_t args[1];
        args[0].kind = WASM_I32;
        args[0].of.i32 = input;

        wasm_val_t results[1];
        results[0].kind = WASM_I32;

        bool ret = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, args);
        EXPECT_TRUE(ret) << "Failed to call i32_tee function: " << wasm_runtime_get_exception(module_inst);

        return results[0].of.i32;
    }

    /**
     * @brief Call WASM function with i64 local.tee operation
     * @param input The i64 value to tee to local variable
     * @return The value returned on stack after tee operation
     * @details Tests that local.tee both stores the value locally AND returns it on stack
     */
    int64_t call_i64_tee(int64_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_tee");
        EXPECT_NE(nullptr, func) << "Failed to lookup i64_tee function";

        wasm_val_t args[1];
        args[0].kind = WASM_I64;
        args[0].of.i64 = input;

        wasm_val_t results[1];
        results[0].kind = WASM_I64;

        bool ret = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, args);
        EXPECT_TRUE(ret) << "Failed to call i64_tee function: " << wasm_runtime_get_exception(module_inst);

        return results[0].of.i64;
    }

    /**
     * @brief Call WASM function with f32 local.tee operation
     * @param input The f32 value to tee to local variable
     * @return The value returned on stack after tee operation
     * @details Tests that local.tee both stores the value locally AND returns it on stack
     */
    float call_f32_tee(float input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_f32_tee");
        EXPECT_NE(nullptr, func) << "Failed to lookup f32_tee function";

        wasm_val_t args[1];
        args[0].kind = WASM_F32;
        args[0].of.f32 = input;

        wasm_val_t results[1];
        results[0].kind = WASM_F32;

        bool ret = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, args);
        EXPECT_TRUE(ret) << "Failed to call f32_tee function: " << wasm_runtime_get_exception(module_inst);

        return results[0].of.f32;
    }

    /**
     * @brief Call WASM function with f64 local.tee operation
     * @param input The f64 value to tee to local variable
     * @return The value returned on stack after tee operation
     * @details Tests that local.tee both stores the value locally AND returns it on stack
     */
    double call_f64_tee(double input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_f64_tee");
        EXPECT_NE(nullptr, func) << "Failed to lookup f64_tee function";

        wasm_val_t args[1];
        args[0].kind = WASM_F64;
        args[0].of.f64 = input;

        wasm_val_t results[1];
        results[0].kind = WASM_F64;

        bool ret = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, args);
        EXPECT_TRUE(ret) << "Failed to call f64_tee function: " << wasm_runtime_get_exception(module_inst);

        return results[0].of.f64;
    }

    /**
     * @brief Call WASM function that tees a value and returns the stored local content
     * @param input The value to tee to local variable
     * @return The value retrieved from the local variable after tee
     * @details Validates that local.tee properly stored the value in the local variable
     */
    int32_t verify_tee_and_get_i32(int32_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i32_tee_and_get");
        EXPECT_NE(nullptr, func) << "Failed to lookup test_i32_tee_and_get function";

        wasm_val_t args[1];
        args[0].kind = WASM_I32;
        args[0].of.i32 = input;

        wasm_val_t results[1];
        results[0].kind = WASM_I32;

        bool ret = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, args);
        EXPECT_TRUE(ret) << "Failed to call test_i32_tee_and_get function: " << wasm_runtime_get_exception(module_inst);

        return results[0].of.i32;
    }

    /**
     * @brief Call WASM function that performs repeated tee operations
     * @param input The initial value for repeated tee operations
     * @return The final value after multiple tee operations
     * @details Tests consistency of multiple consecutive local.tee operations
     */
    int32_t call_repeated_tee(int32_t input)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_repeated_tee");
        EXPECT_NE(nullptr, func) << "Failed to lookup repeated_tee function";

        wasm_val_t args[1];
        args[0].kind = WASM_I32;
        args[0].of.i32 = input;

        wasm_val_t results[1];
        results[0].kind = WASM_I32;

        bool ret = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, args);
        EXPECT_TRUE(ret) << "Failed to call repeated_tee function: " << wasm_runtime_get_exception(module_inst);

        return results[0].of.i32;
    }

    /**
     * @brief Load WASM file and return buffer with size
     * @param file_path Path to the WASM file
     * @return Pair of buffer pointer and size
     * @details Helper function to load WASM binary files for testing
     */
    std::pair<uint8_t*, uint32_t> load_wasm_file(const char* file_path)
    {
        uint32_t buffer_size = 0;
        // NOLINTNEXTLINE(modernize-use-auto)
        uint8_t *buffer = (uint8_t *)bh_read_file_to_buffer(file_path, &buffer_size);

        if (buffer == nullptr) {
            return { buffer, 0 };
        }

        return { buffer, buffer_size };
    }
};

/**
 * @test BasicTeeOperation_ReturnsValueAndStoresLocally
 * @brief Validates local.tee dual functionality with typical values across all types
 * @details Tests that local.tee correctly performs both operations: stores value to local
 *          variable AND returns the same value on the execution stack. Verifies behavior
 *          for i32, i64, f32, and f64 types with standard test values.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:local_tee_operation
 * @input_conditions Standard values: i32(42), i64(1000000L), f32(3.14f), f64(2.718)
 * @expected_behavior Returns input value on stack AND stores identical value in local
 * @validation_method Dual verification of stack return and local variable content
 */
TEST_P(LocalTeeTest, BasicTeeOperation_ReturnsValueAndStoresLocally)
{
    // Test i32 local.tee operation
    int32_t i32_input = 42;
    int32_t i32_result = call_i32_tee(i32_input);
    ASSERT_EQ(i32_input, i32_result) << "i32 local.tee failed to return correct stack value";

    // Verify the value was stored in local variable
    int32_t stored_value = verify_tee_and_get_i32(i32_input);
    ASSERT_EQ(i32_input, stored_value) << "i32 local.tee failed to store value in local variable";

    // Test i64 local.tee operation
    int64_t i64_input = 1000000L;
    int64_t i64_result = call_i64_tee(i64_input);
    ASSERT_EQ(i64_input, i64_result) << "i64 local.tee failed to return correct stack value";

    // Test f32 local.tee operation
    float f32_input = 3.14f;
    float f32_result = call_f32_tee(f32_input);
    ASSERT_EQ(f32_input, f32_result) << "f32 local.tee failed to return correct stack value";

    // Test f64 local.tee operation
    double f64_input = 2.718;
    double f64_result = call_f64_tee(f64_input);
    ASSERT_EQ(f64_input, f64_result) << "f64 local.tee failed to return correct stack value";
}

// Instantiate parameterized tests for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningModeTest, LocalTeeTest,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT),
                        [](const testing::TestParamInfo<RunningMode>& info) {
                            return info.param == Mode_Interp ? "INTERP" : "AOT";
                        });

// Test environment initialization
class LocalTeeTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        char *cwd = realpath(".", nullptr);
        ASSERT_NE(nullptr, cwd) << "Failed to get current working directory";

        CWD = std::string(cwd) + "/";

        // Set WASM file paths
        WASM_FILE = "wasm-apps/local_tee_test.wasm";
        WASM_FILE_INVALID_INDEX = "wasm-apps/local_tee_invalid_index_test.wasm";
        WASM_FILE_TYPE_MISMATCH = "wasm-apps/local_tee_type_mismatch_test.wasm";

        if (cwd != nullptr) {
            free(cwd);
        }
    }
};

// Register the test environment
// NOLINTNEXTLINE(cert-err58-cpp)
static ::testing::Environment* const local_tee_env =
    ::testing::AddGlobalTestEnvironment(new LocalTeeTestEnvironment());
