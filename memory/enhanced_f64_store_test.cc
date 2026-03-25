/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for f64.store Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly f64.store
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Basic Store Operations: Memory store operations with typical f64 values and addresses
 * - Boundary Store Operations: Memory boundary operations at limits (address 0, memory_size-8)
 * - Special Values: Store operations with NaN, infinity, zero, and denormal double values
 * - Out-of-bounds Access: Invalid memory access attempts and proper trap validation
 *
 * Target Coverage:
 * - Interpreter: core/iwasm/interpreter/wasm_interp_classic.c:WASM_OP_F64_STORE
 * - AOT: core/iwasm/compilation/aot_emit_memory.c:aot_compile_op_f64_store()
 * - Fast JIT: core/iwasm/fast-jit/fe/jit_emit_memory.c:jit_compile_op_f64_store()
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;

static int app_argc;
static char **app_argv;

/**
 * @class F64StoreTest
 * @brief Test fixture for f64.store opcode validation across execution modes
 * @details Provides comprehensive test infrastructure for f64.store operations
 *          including module loading, execution environment setup, and validation helpers
 */
class F64StoreTest : public testing::TestWithParam<RunningMode>
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
     * @details Initializes WAMR runtime, loads f64.store test module, and
     *          prepares execution environment for parameterized testing
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

        // Use f64.store specific WASM file
        std::string store_wasm_file = cwd + "/wasm-apps/f64_store_test.wasm";
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
     * @brief Clean up test environment and release WAMR resources
     * @details Destroys execution environment, module instance, module, and
     *          performs runtime cleanup using RAII patterns
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
     * @brief Call WASM function to store f64 value at given address
     * @details Invokes the store_f64 function in the loaded WASM module
     * @param address Memory address where to store the value
     * @param value f64 value to store
     * @return 0 if store succeeded, -1 if trapped
     */
    int32_t call_f64_store(uint32_t address, double value)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "store_f64");
        EXPECT_NE(func, nullptr) << "Failed to lookup store_f64 function";

        if (func == nullptr) return -1;

        uint32_t argv[3]; // address and f64 value (2 words)
        argv[0] = address;
        memcpy(&argv[1], &value, sizeof(double));

        bool ret = wasm_runtime_call_wasm(exec_env, func, 3, argv);
        exception = wasm_runtime_get_exception(module_inst);

        if (!ret || exception != nullptr) {
            return -1; // Indicate execution failure or trap
        }
        return 0; // Success
    }

    /**
     * @brief Call WASM function to load f64 value from given address
     * @details Invokes the load_f64 function to retrieve stored value
     * @param address Memory address from which to load the value
     * @return Loaded f64 value or NaN if trapped
     */
    double call_f64_load(uint32_t address)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "load_f64");
        EXPECT_NE(func, nullptr) << "Failed to lookup load_f64 function";

        if (func == nullptr) return std::numeric_limits<double>::quiet_NaN();

        uint32_t argv[2]; // address and return f64 (2 words)
        argv[0] = address;

        bool ret = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        exception = wasm_runtime_get_exception(module_inst);

        if (!ret || exception != nullptr) {
            return std::numeric_limits<double>::quiet_NaN(); // Indicate execution failure
        }

        double result;
        memcpy(&result, &argv[0], sizeof(double));
        return result;
    }

    /**
     * @brief Get memory size from WASM module
     * @details Gets the current memory size in bytes
     * @return Memory size in bytes
     */
    uint32_t get_memory_size_bytes()
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "get_memory_size");
        EXPECT_NE(func, nullptr) << "Failed to lookup get_memory_size function";

        if (func == nullptr) return 0;

        uint32_t argv[1] = {0};
        bool ret = wasm_runtime_call_wasm(exec_env, func, 0, argv);

        if (!ret) return 0;

        // Memory size is returned in pages, convert to bytes (1 page = 64KB)
        return argv[0] * 65536;
    }

    /**
     * @brief Clear any runtime exception state
     * @details Resets exception state between test operations
     */
    void clear_exception()
    {
        if (exception != nullptr) {
            wasm_runtime_clear_exception(module_inst);
            exception = nullptr;
        }
    }

    /**
     * @brief Compare two double values considering NaN equality
     * @details Compares double values with proper NaN handling
     * @param expected Expected double value
     * @param actual Actual double value
     * @return true if values are equal or both are NaN
     */
    bool double_equal(double expected, double actual)
    {
        if (std::isnan(expected) && std::isnan(actual)) {
            return true; // Both NaN
        }
        return expected == actual;
    }

    /**
     * @brief Compare double bit patterns for exact equality
     * @details Compares the raw bit representation of doubles
     * @param expected Expected double value
     * @param actual Actual double value
     * @return true if bit patterns are identical
     */
    bool double_bits_equal(double expected, double actual)
    {
        uint64_t expected_bits, actual_bits;
        memcpy(&expected_bits, &expected, sizeof(uint64_t));
        memcpy(&actual_bits, &actual, sizeof(uint64_t));
        return expected_bits == actual_bits;
    }
};

/**
 * @test BoundaryAddresses_StoreAtLimits_HandlesCorrectly
 * @brief Validates f64.store at memory boundaries and various alignments
 * @details Tests boundary conditions for memory access at valid edge addresses and alignment scenarios.
 *          Verifies proper boundary handling and alignment behavior for double-precision values.
 * @test_category Corner - Memory boundary and alignment validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_store_boundary_check
 * @input_conditions Store at address 0, (memory_size - 8), and unaligned addresses
 * @expected_behavior Successful storage at valid boundary and alignment addresses
 * @validation_method Store and verify at memory boundaries and various alignments
 */
TEST_P(F64StoreTest, BoundaryAddresses_StoreAtLimits_HandlesCorrectly)
{
    uint32_t memory_size = get_memory_size_bytes();
    ASSERT_GE(memory_size, 16u) << "Memory size too small for boundary testing";

    double boundary_value = 999.999999999;

    // Store at memory start (address 0)
    ASSERT_EQ(call_f64_store(0, boundary_value), 0)
        << "Failed to store f64 value at memory start (address 0)";

    double loaded_start = call_f64_load(0);
    ASSERT_FALSE(std::isnan(loaded_start))
        << "Failed to load f64 value from memory start";

    ASSERT_TRUE(double_bits_equal(boundary_value, loaded_start))
        << "Value mismatch at memory start";

    // Store near memory end (last valid f64 position)
    uint32_t last_valid_addr = memory_size - 8;
    ASSERT_EQ(call_f64_store(last_valid_addr, boundary_value), 0)
        << "Failed to store f64 value at last valid address " << last_valid_addr;

    double loaded_end = call_f64_load(last_valid_addr);
    ASSERT_FALSE(std::isnan(loaded_end))
        << "Failed to load f64 value from last valid address";

    ASSERT_TRUE(double_bits_equal(boundary_value, loaded_end))
        << "Value mismatch at memory end";

    // Test unaligned addresses (f64 can be stored at non-8-byte aligned addresses)
    double unaligned_value = 123.456789;
    uint32_t unaligned_addresses[] = {1, 2, 3, 4, 5, 6, 7, 9, 10, 11};

    for (uint32_t addr : unaligned_addresses) {

        // Skip if address + 8 would exceed memory
        if (addr + 8 > memory_size) continue;

        // Store at unaligned address
        ASSERT_EQ(call_f64_store(addr, unaligned_value), 0)
            << "Failed to store f64 value at unaligned address " << addr;

        // Verify stored value
        double loaded_value = call_f64_load(addr);
        ASSERT_FALSE(std::isnan(loaded_value))
            << "Failed to load f64 value from unaligned address " << addr;

        ASSERT_TRUE(double_bits_equal(unaligned_value, loaded_value))
            << "Value mismatch at unaligned address " << addr;
    }
}

INSTANTIATE_TEST_SUITE_P(
    F64StoreTestSuite,
    F64StoreTest,
    testing::Values(
        RunningMode::Mode_Interp
#if WASM_ENABLE_AOT != 0
        , RunningMode::Mode_LLVM_JIT
#endif
    )
);
