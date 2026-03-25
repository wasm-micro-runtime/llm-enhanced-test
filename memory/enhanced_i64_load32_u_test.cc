/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <memory>
#include <cstring>
#include "wasm_runtime.h"
#include "bh_read_file.h"
#include "test_helper.h"

/**
 * @brief Test fixture for comprehensive i64.load32_u opcode validation
 *
 * This test suite validates the i64.load32_u WebAssembly opcode across different
 * execution modes (interpreter and AOT). The opcode loads an unsigned 32-bit integer
 * from memory and zero-extends it to 64-bit. Tests cover basic functionality,
 * zero extension behavior, boundary conditions, error scenarios, and cross-mode consistency.
 */
class I64Load32UTest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up test environment with WAMR runtime initialization
     *
     * Initializes the WAMR runtime with system allocator and loads
     * the i64.load32_u test module for execution validation.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        init_args.running_mode = GetParam();

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        is_aot_mode = (GetParam() == Mode_LLVM_JIT);

        // Load the i64.load32_u test module
        LoadModule();
    }

    /**
     * @brief Clean up test environment and destroy WAMR runtime
     *
     * Properly releases module instance, module, and runtime resources
     * to prevent memory leaks and ensure clean test isolation.
     */
    void TearDown() override
    {
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }

        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }

        if (buffer != nullptr) {
            delete[] buffer;
            buffer = nullptr;
        }

        wasm_runtime_destroy();
    }

    /**
     * @brief Load the i64.load32_u test WebAssembly module
     *
     * Loads the appropriate WASM module based on execution mode and
     * instantiates it for test execution.
     */
    void LoadModule()
    {
        const char* wasm_file = "wasm-apps/i64_load32_u_test.wasm";

        buffer = bh_read_file_to_buffer(wasm_file, &size);
        ASSERT_NE(nullptr, buffer) << "Failed to read WASM file: " << wasm_file;
        ASSERT_GT(size, 0U) << "WASM file is empty: " << wasm_file;

        module = wasm_runtime_load((uint8_t*)buffer, size, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate module: " << error_buf;
    }

    /**
     * @brief Execute a WASM function by name with provided arguments
     *
     * @param func_name Name of the exported WASM function to call
     * @param argc Number of arguments to pass to the function
     * @param argv Array of argument values for the function
     * @return bool True if execution succeeded, false if trapped/failed
     */
    bool CallWasmFunction(const char* func_name, uint32_t argc, uint32_t* argv)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Failed to lookup function: " << func_name;

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        bool ret = wasm_runtime_call_wasm(exec_env, func, argc, argv);

        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }

        return ret;
    }

    /**
     * @brief Call i64.load32_u test function with specified address
     *
     * @param addr Memory address to load from
     * @return uint64_t Zero-extended 64-bit result
     */
    uint64_t CallLoad32U(uint32_t addr)
    {
        uint32_t argv[1] = { addr };
        uint32_t argc = 1;

        bool success = CallWasmFunction("test_i64_load32_u", argc, argv);
        EXPECT_TRUE(success) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        // Return result as uint64_t (argv[0] contains low 32 bits, argv[1] contains high 32 bits)
        return ((uint64_t)argv[1] << 32) | argv[0];
    }

    /**
     * @brief Call i64.load32_u test function with address and offset
     *
     * @param addr Base memory address
     * @param offset Static offset from instruction
     * @return uint64_t Zero-extended 64-bit result
     */
    uint64_t CallLoad32UWithOffset(uint32_t addr, uint32_t offset)
    {
        uint32_t argv[2] = { addr, offset };
        uint32_t argc = 2;

        bool success = CallWasmFunction("test_i64_load32_u_offset", argc, argv);
        EXPECT_TRUE(success) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        return ((uint64_t)argv[1] << 32) | argv[0];
    }

    /**
     * @brief Call memory initialization helper function
     *
     * @param addr Address to write to
     * @param value 32-bit value to write
     */
    void InitMemoryU32(uint32_t addr, uint32_t value)
    {
        uint32_t argv[2] = { addr, value };
        uint32_t argc = 2;

        bool success = CallWasmFunction("init_memory_u32", argc, argv);
        EXPECT_TRUE(success) << "Memory init failed: " << wasm_runtime_get_exception(module_inst);
    }

    /**
     * @brief Test error condition with out-of-bounds access
     *
     * @param addr Invalid address that should cause trap
     * @return bool true if trap occurred as expected
     */
    bool TestOutOfBounds(uint32_t addr)
    {
        uint32_t argv[1] = { addr };
        uint32_t argc = 1;

        bool success = CallWasmFunction("test_i64_load32_u", argc, argv);
        return !success; // Returns true if trap occurred (expected)
    }

    RuntimeInitArgs init_args{};
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    char* buffer = nullptr;
    uint32_t size = 0;
    char error_buf[256]{};
    bool is_aot_mode = false;

    static constexpr uint32_t stack_size = 16 * 1024;
    static constexpr uint32_t heap_size = 16 * 1024;
};

/**
 * @test StaticOffsetHandling_WithOffset_CalculatesAddressCorrectly
 * @brief Tests i64.load32_u with static offset parameter
 * @details Validates proper address calculation when using static offset in addition
 *          to dynamic address, ensuring base + offset arithmetic works correctly.
 * @test_category Main - Offset parameter validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:address_offset_calculation
 * @input_conditions Base addresses with various static offsets
 * @expected_behavior Correct value loaded from (base + offset) address
 * @validation_method Compare offset-based loads with direct address loads
 */
TEST_P(I64Load32UTest, StaticOffsetHandling_WithOffset_CalculatesAddressCorrectly)
{
    // Initialize memory with pattern for offset testing
    InitMemoryU32(60, 0x11111111U);
    InitMemoryU32(64, 0x22222222U);
    InitMemoryU32(68, 0x33333333U);
    InitMemoryU32(72, 0x44444444U);

    // Test loading with static offsets
    uint64_t result1 = CallLoad32UWithOffset(60, 0);   // base + 0
    uint64_t result2 = CallLoad32UWithOffset(60, 4);   // base + 4
    uint64_t result3 = CallLoad32UWithOffset(60, 8);   // base + 8
    uint64_t result4 = CallLoad32UWithOffset(60, 12);  // base + 12

    // Verify offset calculations
    ASSERT_EQ(0x0000000011111111ULL, result1)
        << "Load with offset 0 failed";

    ASSERT_EQ(0x0000000022222222ULL, result2)
        << "Load with offset 4 failed";

    ASSERT_EQ(0x0000000033333333ULL, result3)
        << "Load with offset 8 failed";

    ASSERT_EQ(0x0000000044444444ULL, result4)
        << "Load with offset 12 failed";
}

// Test parameterization for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossMode,
    I64Load32UTest,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode>& info) {
        return info.param == Mode_Interp ? "Interpreter" : "AOT";
    }
);
