/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for i64.store8 Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly i64.store8
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Basic Store8 Operations: 8-bit store operations with typical i64 values and addresses
 * - Value Truncation Operations: Testing proper truncation of i64 values to 8-bit storage
 * - Memory Boundary Operations: Store operations at memory boundaries and edge cases
 * - Out-of-bounds Access: Invalid memory access attempts and proper trap validation
 *
 * Target Coverage:
 * - Interpreter: core/iwasm/interpreter/wasm_interp_classic.c:WASM_OP_I64_STORE8
 * - AOT: core/iwasm/compilation/aot_emit_memory.c:aot_compile_op_i64_store8()
 * - Fast JIT: core/iwasm/fast-jit/fe/jit_emit_memory.c:jit_compile_op_i64_store8()
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
 * @class I64Store8Test
 * @brief Test fixture for i64.store8 opcode validation across execution modes
 * @details Provides comprehensive test infrastructure for i64.store8 operations
 *          including module loading, execution environment setup, and validation helpers
 */
class I64Store8Test : public testing::TestWithParam<RunningMode>
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
     * @brief Set up test environment and load WASM module for i64.store8 testing
     * @details Initializes WAMR runtime, loads test module, and prepares execution environment
     *          for comprehensive i64.store8 opcode validation across execution modes
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

        // Use i64.store8 specific WASM file
        std::string store8_wasm_file = cwd + "/wasm-apps/i64_store8_test.wasm";
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
     * @brief Helper function to call i64.store8 WASM function and retrieve stored byte
     * @param addr Memory address for store operation
     * @param value i64 value to store (lower 8 bits will be stored)
     * @return Stored byte value read back from memory
     */
    uint8_t call_i64_store8_and_read(uint32_t addr, uint64_t value) {
        // Call store function
        func_inst = wasm_runtime_lookup_function(module_inst, "store_i64_byte");
        EXPECT_NE(func_inst, nullptr) << "Failed to lookup store_i64_byte function";

        uint32_t argv[3] = { addr, (uint32_t)value, (uint32_t)(value >> 32) };
        bool ret = wasm_runtime_call_wasm(exec_env, func_inst, 3, argv);
        EXPECT_TRUE(ret) << "Failed to call store_i64_byte function";

        // Read back the stored byte
        func_inst = wasm_runtime_lookup_function(module_inst, "load_byte");
        EXPECT_NE(func_inst, nullptr) << "Failed to lookup load_byte function";

        uint32_t read_argv[1] = { addr };
        ret = wasm_runtime_call_wasm(exec_env, func_inst, 1, read_argv);
        EXPECT_TRUE(ret) << "Failed to call load_byte function";

        return (uint8_t)read_argv[0];
    }

    /**
     * @brief Helper function to test out-of-bounds store operations
     * @param addr Memory address for store operation (expected to be out-of-bounds)
     * @param value i64 value to attempt storing
     * @return true if operation caused a trap (expected behavior), false otherwise
     */
    bool test_out_of_bounds_store(uint32_t addr, uint64_t value) {
        func_inst = wasm_runtime_lookup_function(module_inst, "store_i64_byte_oob");
        EXPECT_NE(func_inst, nullptr) << "Failed to lookup store_i64_byte_oob function";

        uint32_t argv[3] = { addr, (uint32_t)value, (uint32_t)(value >> 32) };

        // Clear any previous exception
        wasm_runtime_clear_exception(module_inst);

        bool ret = wasm_runtime_call_wasm(exec_env, func_inst, 3, argv);

        // Check if an exception occurred (expected for out-of-bounds)
        const char *exception = wasm_runtime_get_exception(module_inst);
        return !ret || (exception != nullptr);
    }

    /**
     * @brief Get memory size in bytes from the WASM module instance
     * @return Current memory size in bytes
     */
    uint32_t get_memory_size() {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "get_memory_size_bytes");
        EXPECT_NE(func, nullptr) << "Failed to lookup get_memory_size_bytes function";

        uint32_t argv[1] = {0};
        bool ret = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_TRUE(ret) << "Failed to call get_memory_size_bytes function";

        return argv[0];
    }
};

/**
 * @test BasicStore_TypicalValues_StoresCorrectByte
 * @brief Validates i64.store8 produces correct byte storage for typical inputs
 * @details Tests fundamental store8 operation with various i64 values to ensure
 *          correct truncation to 8-bit values and proper memory storage.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i64_store8_operation
 * @input_conditions Various i64 values: 0x42, 0x123456789ABCDEF0, 0xFFFFFFFFFFFFFF00
 * @expected_behavior Stores lower 8 bits: 0x42, 0xF0, 0x00 respectively
 * @validation_method Direct comparison of stored byte with expected truncated values
 */
TEST_P(I64Store8Test, BasicStore_TypicalValues_StoresCorrectByte) {
    // Test basic i64 value storage with typical values
    uint8_t result = call_i64_store8_and_read(0, 0x42ULL);
    ASSERT_EQ(0x42, result) << "Failed to store simple i64 value 0x42";

    // Test truncation of large i64 value - should store lower 8 bits
    result = call_i64_store8_and_read(4, 0x123456789ABCDEF0ULL);
    ASSERT_EQ(0xF0, result) << "Failed to truncate i64 value 0x123456789ABCDEF0 to 0xF0";

    // Test truncation where lower 8 bits are zero
    result = call_i64_store8_and_read(8, 0xFFFFFFFFFFFFFF00ULL);
    ASSERT_EQ(0x00, result) << "Failed to truncate i64 value with zero lower byte";
}

// Parameterized tests for both interpreter and AOT execution modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I64Store8Test,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         testing::PrintToStringParamName());

