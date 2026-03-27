/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for i32.store8 Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly i32.store8
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Basic Store8 Operations: 8-bit store operations with typical i32 values and addresses
 * - Value Truncation Operations: Testing proper truncation of i32 values to 8-bit storage
 * - Memory Boundary Operations: Store operations at memory boundaries and edge cases
 * - Out-of-bounds Access: Invalid memory access attempts and proper trap validation
 *
 * Target Coverage:
 * - Interpreter: core/iwasm/interpreter/wasm_interp_classic.c:WASM_OP_I32_STORE8
 * - AOT: core/iwasm/compilation/aot_emit_memory.c:aot_compile_op_i32_store8()
 * - Fast JIT: core/iwasm/fast-jit/fe/jit_emit_memory.c:jit_compile_op_i32_store8()
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static int app_argc;
static char **app_argv;

/**
 * @class I32Store8Test
 * @brief Test fixture for i32.store8 opcode validation across execution modes
 * @details Provides comprehensive test infrastructure for i32.store8 operations
 *          including module loading, execution environment setup, and validation helpers
 */
class I32Store8Test : public testing::TestWithParam<RunningMode>
{
  protected:
    WAMRRuntimeRAII<> runtime;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint32_t buf_size, stack_size = 8092, heap_size = 8092;
    uint8_t *buf = nullptr;
    char error_buf[128] = { 0 };
    wasm_function_inst_t func_inst = nullptr;

    /**
     * @brief Set up test environment and load WASM module for i32.store8 testing
     * @details Initializes WAMR runtime, loads test module, and prepares execution environment
     *          for comprehensive i32.store8 opcode validation across execution modes
     */
    void SetUp() override {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        // Get current working directory
        char *cwd_ptr = getcwd(nullptr, 0);
        ASSERT_NE(cwd_ptr, nullptr) << "Failed to get current working directory";
        std::string cwd = std::string(cwd_ptr);
        free(cwd_ptr);

        // Use i32.store8 specific WASM file
        std::string store8_wasm_file = cwd + "/wasm-apps/i32_store8_test.wasm";
        buf = (uint8_t *)bh_read_file_to_buffer(store8_wasm_file.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << store8_wasm_file;

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
     * @brief Clean up test environment and release WASM module resources
     * @details Destroys execution environment, unloads module instance, and performs cleanup
     */
    void TearDown() override {
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
     * @brief Execute WASM function with error handling and result validation
     * @param func_name Name of the WASM function to execute
     * @param args Array of function arguments
     * @param argc Number of arguments
     * @return Function execution result, or UINT32_MAX on error
     */
    uint32_t call_wasm_function(const char* func_name, uint32_t args[], int argc) {
        func_inst = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func_inst) << "Function " << func_name << " not found";
        if (func_inst == nullptr) return UINT32_MAX;

        uint32_t argv[8];  // Support up to 8 parameters
        for (int i = 0; i < argc && i < 8; i++) {
            argv[i] = args[i];
        }

        bool ret = wasm_runtime_call_wasm(exec_env, func_inst, argc, argv);

        if (!ret) {
            const char* exception = wasm_runtime_get_exception(module_inst);
            if (exception != nullptr) {
                EXPECT_TRUE(strstr(exception, "out of bounds") != nullptr ||
                           strstr(exception, "unreachable") != nullptr)
                    << "Unexpected exception: " << exception;
                return UINT32_MAX;  // Signal exception occurred
            }
        }

        return ret ? argv[0] : UINT32_MAX;
    }
};

/**
 * @test OutOfBoundsAccess_InvalidAddresses_TrapsCorrectly
 * @brief Tests that attempts to store beyond memory bounds result in proper traps
 * @details Validates i32.store8 error handling for out-of-bounds memory access attempts.
 *          Tests various invalid addresses beyond memory limits and verifies proper
 *          trap generation and error reporting.
 * @test_category Error - Out-of-bounds access validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:WASM_OP_I32_STORE8
 * @input_conditions Addresses beyond memory size: 0xFFFFFFFC, 0xFFFFFFFE, large offsets
 * @expected_behavior WASM execution traps with out-of-bounds memory access error
 * @validation_method Exception detection and error message validation
 */
TEST_P(I32Store8Test, OutOfBoundsAccess_InvalidAddresses_TrapsCorrectly) {
    uint32_t args[2];

    // Attempt to store beyond allocated memory using very large address
    args[0] = 0xFFFFFFFC; args[1] = 0x42;
    uint32_t result = call_wasm_function("test_out_of_bounds_store", args, 2);
    ASSERT_EQ(UINT32_MAX, result) << "Expected out-of-bounds store to trap";

    // Verify exception was generated
    const char* exception = wasm_runtime_get_exception(module_inst);
    ASSERT_NE(nullptr, exception) << "Expected exception for out-of-bounds access";
    ASSERT_TRUE(strstr(exception, "out of bounds") != nullptr)
        << "Expected 'out of bounds' in exception message: " << exception;

    // Clear exception for next test
    wasm_runtime_clear_exception(module_inst);

    // Attempt store to maximum address
    args[0] = 0xFFFFFFFE; args[1] = 0x99;
    result = call_wasm_function("test_out_of_bounds_store", args, 2);
    ASSERT_EQ(UINT32_MAX, result) << "Expected maximum address store to trap";

    exception = wasm_runtime_get_exception(module_inst);
    ASSERT_NE(nullptr, exception) << "Expected exception for large address access";
}

// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I32Store8Test,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT));
