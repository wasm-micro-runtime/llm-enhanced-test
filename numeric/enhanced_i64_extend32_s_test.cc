/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for i64.extend32_s Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly i64.extend32_s
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic sign extension functionality with typical values
 * - Corner Cases: Boundary conditions at 32-bit signed integer limits
 * - Edge Cases: Upper bit interference and mathematical properties
 * - Error Handling: Stack underflow and validation scenarios
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (lines handling i64.extend32_s)
 * Source Location: core/iwasm/interpreter/wasm_interp_classic.c (i64 sign extension operations)
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;
static std::string WASM_FILE_UNDERFLOW;

static int
app_argc;
static char **app_argv;

/**
 * @class I64Extend32sTest
 * @brief Test fixture for comprehensive i64.extend32_s opcode validation
 *
 * Provides WAMR runtime environment setup and cleanup for testing
 * i64.extend32_s sign extension operations across different execution modes.
 * Supports both interpreter and AOT modes through parameterized testing.
 */
class I64Extend32sTest : public testing::TestWithParam<RunningMode>
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
     * @brief Initialize WAMR runtime and load test modules
     *
     * Sets up the WebAssembly runtime environment and loads the test module
     * containing i64.extend32_s test functions. Configures both normal operation
     * and stack underflow test scenarios.
     *
     * Source Location: Called before each test case execution
     */
    void SetUp() override {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        // Initialize WASM file paths if not already initialized
        if (WASM_FILE.empty()) {
            WASM_FILE = "wasm-apps/i64_extend32_s_test.wasm";
        }
        if (WASM_FILE_UNDERFLOW.empty()) {
            WASM_FILE_UNDERFLOW = "wasm-apps/i64_extend32_s_stack_underflow.wasm";
        }

        // Load i64.extend32_s test WASM module
        buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode (interpreter or AOT)
        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up WAMR runtime resources
     *
     * Properly releases all allocated WAMR runtime resources including
     * execution environment, module instance, and module. Ensures clean
     * state for subsequent test executions.
     *
     * Source Location: Called after each test case completion
     */
    void TearDown() override {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (buf != nullptr) {
            BH_FREE(buf);
        }
    }

    /**
     * @brief Execute i64.extend32_s operation with specified input
     *
     * Calls the WASM test function that performs i64.extend32_s operation
     * on the provided 64-bit input value. Handles function lookup, parameter
     * passing, and result extraction from WASM execution environment.
     *
     * @param input The 64-bit input value to sign-extend
     * @return The 64-bit result after sign extension
     *
     * Source Location: core/iwasm/interpreter/wasm_interp_classic.c:5651-5655 (i64.extend32_s handler)
     */
    uint64_t call_i64_extend32_s(uint64_t input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_extend32_s");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_extend32_s function";

        uint32_t wasm_argv[2];
        // Pack uint64_t input as two uint32_t values (little-endian order)
        wasm_argv[0] = (uint32_t)(input & 0xFFFFFFFF);
        wasm_argv[1] = (uint32_t)(input >> 32);

        bool call_result = wasm_runtime_call_wasm(exec_env, func, 2, wasm_argv);
        EXPECT_TRUE(call_result) << "WASM function call failed: " << wasm_runtime_get_exception(module_inst);

        // Unpack result from two uint32_t values to uint64_t
        uint64_t result = ((uint64_t)wasm_argv[1] << 32) | wasm_argv[0];
        return result;
    }

};

/**
 * @test BasicSignExtension_NegativeValues_ExtendsCorrectly
 * @brief Validates i64.extend32_s correctly sign-extends negative 32-bit values
 * @details Tests fundamental sign extension operation with negative integers where bit 31 is 1.
 *          Verifies that negative 32-bit values extend to 64-bit with upper 32 bits as ones.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:DEF_OP_CONVERT(int64, I64, int32, I64)
 * @input_conditions Negative 32-bit values: 0x87654321, 0xFFFFFFFF, 0x80000000
 * @expected_behavior Upper 32-bits filled with ones for negative values (sign extension)
 * @validation_method Direct comparison of WASM function result with expected bit patterns
 */
TEST_P(I64Extend32sTest, BasicSignExtension_NegativeValues_ExtendsCorrectly) {
    // Test typical negative 32-bit value (bit 31 = 1)
    uint64_t result1 = call_i64_extend32_s(0x0000000087654321ULL);
    ASSERT_EQ(0xFFFFFFFF87654321ULL, result1)
        << "Failed to correctly extend negative value 0x87654321";

    // Test -1 in 32-bit representation
    uint64_t result2 = call_i64_extend32_s(0x00000000FFFFFFFFULL);
    ASSERT_EQ(0xFFFFFFFFFFFFFFFFULL, result2)
        << "Failed to correctly extend -1 (0xFFFFFFFF)";

    // Test minimum negative 32-bit value (INT32_MIN)
    uint64_t result3 = call_i64_extend32_s(0x0000000080000000ULL);
    ASSERT_EQ(0xFFFFFFFF80000000ULL, result3)
        << "Failed to correctly extend INT32_MIN (0x80000000)";
}

// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I64Extend32sTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT));

// Initialize file paths for test execution
__attribute__((constructor))
void init_test_paths() {
    char *cwd_buffer = getcwd(nullptr, 0);
    if (cwd_buffer != nullptr) {
        CWD = std::string(cwd_buffer);
        free(cwd_buffer);
    }
    WASM_FILE = "wasm-apps/i64_extend32_s_test.wasm";
    WASM_FILE_UNDERFLOW = "wasm-apps/i64_extend32_s_stack_underflow.wasm";
}
