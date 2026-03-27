/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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

static std::string CWD;
static std::string WASM_FILE;
static const char *WASM_FILE_1 = "i64_store32_test.wasm";

static RunningMode running_mode_supported[] = { Mode_Interp, Mode_LLVM_JIT };

/**
 * @brief Enhanced test suite class for i64.store32 opcode validation
 * @details Comprehensive testing framework for WASM i64.store32 instruction that stores
 *          the low 32 bits of a 64-bit integer to linear memory at specified addresses.
 *          Tests cover basic functionality, boundary conditions, memory alignment,
 *          value truncation behavior, and error handling across interpreter and AOT modes.
 */
class I64Store32Test : public testing::TestWithParam<RunningMode>
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
     * @brief Set up test environment with WAMR runtime and module loading
     * @details Initializes WAMR runtime, loads test WASM module, and configures
     *          execution environment for both interpreter and AOT modes.
     */
    void SetUp() override
    {
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

        // Use i64.store32 specific WASM file
        std::string store_wasm_file = cwd + "/wasm-apps/i64_store32_test.wasm";
        buf = (uint8_t *)bh_read_file_to_buffer(store_wasm_file.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << store_wasm_file;

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
     * @brief Clean up test environment and resources
     * @details Destroys execution environment, module instance, and unloads module
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
     * @brief Execute WASM function to store i64 value as 32-bit in memory
     * @param address Memory address for store operation
     * @param value 64-bit value to store (will be truncated to 32 bits)
     * @param offset Immediate offset added to address
     * @return Execution result (0 for success, non-zero for failure/trap)
     */
    int32_t call_i64_store32(uint32_t address, uint64_t value, uint32_t offset)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_store32");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_store32 function";

        // WASM function signature: (i32 address, i64 value, i32 offset) -> i32
        // For i64 value, we need to pass it as two 32-bit values (low, high)
        uint32_t argv[4] = {
            address,
            static_cast<uint32_t>(value),           // i64 low 32 bits
            static_cast<uint32_t>(value >> 32),     // i64 high 32 bits
            offset
        };
        uint32_t argc = 4;

        bool ret = wasm_runtime_call_wasm(exec_env, func, argc, argv);
        exception = wasm_runtime_get_exception(module_inst);

        if (!ret || exception != nullptr) {
            return -1; // Indicate execution failure or trap
        }
        return static_cast<int32_t>(argv[0]); // Return function result
    }

    /**
     * @brief Read 32-bit value from memory at specified address
     * @param address Memory address to read from
     * @return 32-bit value stored in memory
     */
    uint32_t read_memory_i32(uint32_t address)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "read_i32");
        EXPECT_NE(func, nullptr) << "Failed to lookup read_i32 function";

        uint32_t argv[1] = { address };
        uint32_t argc = 1;

        bool ret = wasm_runtime_call_wasm(exec_env, func, argc, argv);
        exception = wasm_runtime_get_exception(module_inst);
        EXPECT_TRUE(ret && !exception) << "Failed to read from memory address " << address;
        return argv[0];
    }
};

/**
 * @test OffsetCalculation_HandlesVariousOffsets
 * @brief Validates correct address calculation with different immediate offset values
 * @details Tests i64.store32 with various offset values to ensure proper address calculation
 *          and memory access patterns. Verifies (base_address + offset) computation.
 * @test_category Main - Address calculation validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:address_calculation
 * @input_conditions Different base addresses and offset combinations within memory bounds
 * @expected_behavior Correct memory writes at calculated addresses (base + offset)
 * @validation_method Memory content verification at calculated target addresses
 */
TEST_P(I64Store32Test, OffsetCalculation_HandlesVariousOffsets)
{
    const uint64 test_value = 0xABCDEF0012345678ULL;
    const uint32 expected_stored = 0x12345678U;

    // Test zero offset
    uint32 result = call_i64_store32(100, test_value, 0);
    ASSERT_EQ(0, result) << "Store with zero offset should succeed";
    ASSERT_EQ(expected_stored, read_memory_i32(100))
        << "Zero offset should store at base address";

    // Test small offset
    result = call_i64_store32(100, test_value, 4);
    ASSERT_EQ(0, result) << "Store with small offset should succeed";
    ASSERT_EQ(expected_stored, read_memory_i32(104))
        << "Small offset should store at base+offset address";

    // Test larger offset
    result = call_i64_store32(100, test_value, 50);
    ASSERT_EQ(0, result) << "Store with larger offset should succeed";
    ASSERT_EQ(expected_stored, read_memory_i32(150))
        << "Larger offset should store at base+offset address";

    // Test maximum reasonable offset within memory bounds
    result = call_i64_store32(1000, test_value, 1000);
    ASSERT_EQ(0, result) << "Store with large offset should succeed";
    ASSERT_EQ(expected_stored, read_memory_i32(2000))
        << "Large offset should store at calculated address";
}

INSTANTIATE_TEST_SUITE_P(RunningMode, I64Store32Test,
                         testing::ValuesIn(running_mode_supported),
                         testing::PrintToStringParamName());
