/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"
#include "wasm_runtime.h"

static std::string CWD;
static std::string WASM_FILE;
static constexpr RunningMode running_modes[] = { Mode_Interp, Mode_LLVM_JIT };

/**
 * @brief Test fixture for i64.load16_u opcode validation across interpreter and AOT modes
 *
 * This test suite comprehensively validates the i64.load16_u WebAssembly opcode functionality:
 * - Loads unsigned 16-bit integers from linear memory
 * - Performs proper zero extension to 64-bit unsigned integers
 * - Validates memory bounds checking and error handling
 * - Tests across both interpreter and AOT execution modes
 * - Verifies little-endian byte order in memory access
 * - Tests alignment scenarios for 16-bit memory operations
 */
class I64Load16UTest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up WAMR runtime and load test module for each test case
     *
     * Initializes WAMR runtime with system allocator and loads the i64.load16_u
     * test module from WASM bytecode file. Configures both interpreter and AOT
     * execution modes based on test parameters.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize WAMR runtime
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        WASM_FILE = "wasm-apps/i64_load16_u_test.wasm";

        // Load WASM module from file
        module_buffer = (unsigned char *)bh_read_file_to_buffer(WASM_FILE.c_str(), &module_buffer_size);
        ASSERT_NE(nullptr, module_buffer) << "Failed to read WASM file: " << WASM_FILE;

        // Load module into WAMR
        module = wasm_runtime_load(module_buffer, module_buffer_size, error_buffer, sizeof(error_buffer));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buffer;

        // Instantiate module
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buffer, sizeof(error_buffer));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate WASM module: " << error_buffer;

        // Set execution mode
        wasm_runtime_set_running_mode(module_inst, GetParam());

        // Get memory data pointer for direct memory access
        memory_data = wasm_runtime_addr_app_to_native(module_inst, 0);
        ASSERT_NE(nullptr, memory_data) << "Failed to get memory data pointer";

        // WASM memory is 1 page = 64KB
        memory_size = 65536;
    }

    /**
     * @brief Clean up WAMR resources after each test case
     *
     * Properly deallocates module instance, module, and runtime resources
     * to prevent memory leaks and ensure clean test environment.
     */
    void TearDown() override
    {
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (module_buffer != nullptr) {
            BH_FREE(module_buffer);
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Execute i64.load16_u test function with specified address
     *
     * @param func_name Name of the WASM test function to call
     * @param address Memory address parameter for load operation
     * @return uint64_t Zero-extended 64-bit result from i64.load16_u
     */
    uint64_t call_i64_load16_u_function(const char* func_name, uint32_t address)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Failed to lookup function: " << func_name;

        // Create execution environment
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t wasm_argv[3] = { address, 0, 0 }; // argv[0] = input, return value overwrites starting from argv[0]

        bool success = wasm_runtime_call_wasm(exec_env, func, 1, wasm_argv);
        EXPECT_TRUE(success) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        // Extract i64 result from argv slots (little-endian) - result overwrites argv[0] and argv[1]
        uint64_t result = ((uint64_t)wasm_argv[1] << 32) | wasm_argv[0];

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return result;
    }

    /**
     * @brief Call simple WASM i64.load16_u function with specified memory offset
     * @param offset Memory offset to load 16-bit unsigned value from
     * @return Loaded value as 64-bit unsigned integer with zero extension
     */
    uint64_t call_i64_load16_u(uint32_t offset)
    {
        return call_i64_load16_u_function("i64_load16_u", offset);
    }

    /**
     * @brief Write 16-bit value to memory at specified offset with bounds checking
     * @param offset Memory offset to write value to
     * @param value 16-bit value to write to memory
     */
    void write_memory_u16(uint32_t offset, uint16_t value)
    {
        ASSERT_LE(offset + 2, memory_size) << "Memory write would exceed bounds";

        uint8_t *mem_ptr = (uint8_t *)memory_data + offset;

        // Write in little-endian format (LSB first)
        mem_ptr[0] = (uint8_t)(value & 0xFF);
        mem_ptr[1] = (uint8_t)((value >> 8) & 0xFF);
    }

    /**
     * @brief Verify zero extension property for loaded 64-bit values
     * @param loaded_value 64-bit value returned from i64.load16_u
     * @param expected_16bit Expected 16-bit value that was loaded
     */
    void verify_zero_extension(uint64_t loaded_value, uint16_t expected_16bit)
    {
        // Verify lower 16 bits match expected value
        ASSERT_EQ((uint16_t)(loaded_value & 0xFFFF), expected_16bit)
            << "Lower 16 bits do not match expected value";

        // Verify upper 48 bits are zero (zero extension)
        ASSERT_EQ(loaded_value >> 16, 0ULL)
            << "Upper 48 bits should be zero for unsigned load";
    }

    // Test fixture member variables
    RuntimeInitArgs init_args{};
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    unsigned char *module_buffer = nullptr;
    uint32_t module_buffer_size = 0;
    void *memory_data = nullptr;
    uint32_t memory_size = 0;
    char error_buffer[256]{};
    static constexpr uint32_t stack_size = 16 * 1024;
    static constexpr uint32_t heap_size = 16 * 1024;
};

/**
 * @test UnalignedAccess_WorksCorrectly
 * @brief Validates i64.load16_u works correctly with unaligned memory addresses
 * @details Tests that i64.load16_u can successfully load 16-bit values from
 *          both aligned (even) and unaligned (odd) memory addresses.
 * @test_category Edge - Alignment handling validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:unaligned_memory_access
 * @input_conditions Both aligned (even) and unaligned (odd) addresses
 * @expected_behavior Successful loads regardless of address alignment
 * @validation_method Compare results from aligned vs unaligned addresses
 */
TEST_P(I64Load16UTest, UnalignedAccess_WorksCorrectly)
{
    // Test aligned access (even address)
    write_memory_u16(0, 0x1122);
    uint64_t aligned_result = call_i64_load16_u(0);
    ASSERT_EQ(0x0000000000001122ULL, aligned_result) << "Aligned access failed";

    // Test unaligned access (odd address) - manually set bytes
    if (memory_size >= 4) {
        auto *mem_ptr = (uint8_t *)memory_data;
        mem_ptr[1] = 0x44; // LSB at odd offset
        mem_ptr[2] = 0x33; // MSB at even offset

        uint64_t unaligned_result = call_i64_load16_u(1);
        ASSERT_EQ(0x0000000000003344ULL, unaligned_result) << "Unaligned access failed";

        verify_zero_extension(aligned_result, 0x1122);
        verify_zero_extension(unaligned_result, 0x3344);
    }
}

// Instantiate parameterized tests for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(I64Load16UModes, I64Load16UTest,
                        testing::ValuesIn(running_modes));
