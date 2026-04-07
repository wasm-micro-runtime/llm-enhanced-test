/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;
static int app_argc;
static char **app_argv;

/**
 * @brief Test fixture for i32.gt_s opcode validation
 *
 * This class provides comprehensive testing infrastructure for the i32.gt_s
 * (signed greater than) operation in WebAssembly. The fixture manages WAMR
 * runtime initialization, module loading, and supports both interpreter
 * and AOT execution modes.
 */
class I32GtSTest : public testing::TestWithParam<RunningMode>
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
     * @brief Set up test environment with WAMR runtime initialization
     *
     * Initializes the WAMR runtime with system allocator, loads the test
     * WASM module, and prepares the execution environment for i32.gt_s
     * opcode testing across different execution modes.
     */
    void SetUp() override
    {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        // Ensure WASM file path is set correctly
        if (WASM_FILE.empty()) {
            char *cwd = getcwd(nullptr, 0);
            if (cwd != nullptr) {
                CWD = std::string(cwd);
                free(cwd);
            } else {
                CWD = ".";
            }
            WASM_FILE = CWD + "/wasm-apps/i32_gt_s_test.wasm";
        }

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
     * @brief Clean up test environment and release WAMR resources
     *
     * Properly destroys module instances, unloads modules, and shuts down
     * the WAMR runtime to prevent resource leaks between test executions.
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
     * @brief Execute i32.gt_s comparison with two integer operands
     *
     * @param a First operand (left side of comparison)
     * @param b Second operand (right side of comparison)
     * @return int32_t Result of a > b (1 if true, 0 if false)
     */
    int32_t call_i32_gt_s(int32_t a, int32_t b)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i32_gt_s");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i32_gt_s function";

        uint32_t argv[3] = { (uint32_t)a, (uint32_t)b, 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_EQ(ret, true) << "Function call failed";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Runtime exception occurred: " << (exception != nullptr ? exception : "");

        return (int32_t)argv[0];
    }

    /**
     * @brief Execute boundary test function that validates INT32_MAX vs INT32_MIN
     *
     * @return int32_t Result of INT32_MAX > INT32_MIN comparison
     */
    int32_t call_boundary_test()
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_boundary_comparison");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_boundary_comparison function";

        uint32_t argv[1] = { 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_EQ(ret, true) << "Boundary test function call failed";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Runtime exception in boundary test: " << (exception != nullptr ? exception : "");

        return (int32_t)argv[0];
    }

    /**
     * @brief Execute mathematical property validation function
     *
     * Tests antisymmetry property: if a > b, then !(b > a)
     * @return int32_t 1 if antisymmetry holds, 0 otherwise
     */
    int32_t call_antisymmetry_test()
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_antisymmetry");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_antisymmetry function";

        uint32_t argv[1] = { 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_EQ(ret, true) << "Antisymmetry test function call failed";

        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_EQ(exception, nullptr) << "Runtime exception in antisymmetry test: " << (exception != nullptr ? exception : "");

        return (int32_t)argv[0];
    }
};

/**
 * @test BoundaryValueComparison_HandlesExtremeValues
 * @brief Validates i32.gt_s correctly handles INT32_MIN and INT32_MAX boundary values
 * @details Tests comparison behavior at integer limits to ensure proper signed
 *          arithmetic and boundary condition handling without overflow issues.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i32_gt_s_boundary_handling
 * @input_conditions Boundary values: INT32_MAX, INT32_MIN, adjacent values
 * @expected_behavior Correct signed comparison results at integer extremes
 * @validation_method Boundary value testing with maximum and minimum 32-bit signed integers
 */
TEST_P(I32GtSTest, BoundaryValueComparison_HandlesExtremeValues)
{
    // Test maximum vs minimum: INT32_MAX > INT32_MIN should be true
    ASSERT_EQ(call_i32_gt_s(INT32_MAX, INT32_MIN), 1)
        << "Failed: INT32_MAX > INT32_MIN should return 1 (true)";

    // Test minimum vs maximum: INT32_MIN > INT32_MAX should be false
    ASSERT_EQ(call_i32_gt_s(INT32_MIN, INT32_MAX), 0)
        << "Failed: INT32_MIN > INT32_MAX should return 0 (false)";

    // Test adjacent boundary: (INT32_MAX - 1) > INT32_MAX should be false
    ASSERT_EQ(call_i32_gt_s(INT32_MAX - 1, INT32_MAX), 0)
        << "Failed: (INT32_MAX - 1) > INT32_MAX should return 0 (false)";

    // Test boundary near zero: 1 > 0 should be true
    ASSERT_EQ(call_i32_gt_s(1, 0), 1)
        << "Failed: 1 > 0 should return 1 (true)";

    // Test negative boundary: -1 > INT32_MIN should be true
    ASSERT_EQ(call_i32_gt_s(-1, INT32_MIN), 1)
        << "Failed: -1 > INT32_MIN should return 1 (true)";

    // Test using WASM boundary test function
    ASSERT_EQ(call_boundary_test(), 1)
        << "Failed: WASM boundary test should return 1 for INT32_MAX > INT32_MIN";
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(RunningModeTest, I32GtSTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<I32GtSTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });

// Initialize WASM file paths - this will be called by the main test runner
__attribute__((constructor))
static void init_i32_gt_s_test()
{
    char *cwd = getcwd(nullptr, 0);
    if (cwd != nullptr) {
        CWD = std::string(cwd);
        free(cwd);
    } else {
        CWD = ".";
    }

    WASM_FILE = CWD + "/wasm-apps/i32_gt_s_test.wasm";
}
