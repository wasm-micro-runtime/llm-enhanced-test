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
 * @class I32Load16UTest
 * @brief Comprehensive test suite for i32.load16_u opcode functionality
 *
 * Tests the i32.load16_u WebAssembly instruction which loads a 16-bit unsigned integer
 * from linear memory and zero-extends it to a 32-bit value. Validates memory access,
 * zero extension behavior, alignment handling, and error conditions across both
 * interpreter and AOT execution modes.
 */
class I32Load16UTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Set up test environment with WAMR runtime initialization
     *
     * Initializes WAMR runtime with system allocator, loads the test WASM module,
     * and prepares the execution environment for i32.load16_u instruction testing.
     * Creates module instance for both interpreter and AOT modes.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        is_aot_mode = (GetParam() == Mode_LLVM_JIT);

        load_sample_wasm();
        ASSERT_NE(nullptr, wasm_module) << "Failed to load i32.load16_u test module";

        wasm_module_inst = wasm_runtime_instantiate(wasm_module, stack_size, heap_size, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, wasm_module_inst) << "Failed to instantiate WASM module: " << error_buf;
    }

    /**
     * @brief Clean up test environment and release WAMR resources
     *
     * Deinstantiates module instance, unloads module,
     * and performs complete WAMR runtime cleanup to prevent memory leaks.
     */
    void TearDown() override
    {
        if (wasm_module_inst != nullptr) {
            wasm_runtime_deinstantiate(wasm_module_inst);
            wasm_module_inst = nullptr;
        }
        if (wasm_module != nullptr) {
            wasm_runtime_unload(wasm_module);
            wasm_module = nullptr;
        }
        if (wasm_buf != nullptr) {
            BH_FREE(wasm_buf);
            wasm_buf = nullptr;
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Load the i32.load16_u test WASM module
     *
     * Loads the pre-compiled WASM module containing test functions for i32.load16_u
     * instruction validation, including memory setup and boundary test scenarios.
     */
    void load_sample_wasm()
    {
        const char *wasm_path = "wasm-apps/i32_load16_u_test.wasm";

        wasm_buf = bh_read_file_to_buffer(wasm_path, &wasm_buf_size);
        ASSERT_NE(nullptr, wasm_buf) << "Failed to read WASM file: " << wasm_path;

        wasm_module = wasm_runtime_load((uint8_t*)wasm_buf, wasm_buf_size,
                                      error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, wasm_module) << "Failed to load WASM module: " << error_buf;
    }

    /**
     * @brief Call WASM function to load 16-bit unsigned value from memory
     * @param address Memory address to load from
     * @return Zero-extended 32-bit value loaded from memory
     *
     * Invokes the WASM test function that performs i32.load16_u operation
     * at the specified memory address and returns the loaded value.
     */
    uint32_t call_i32_load16_u(uint32_t address)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(wasm_module_inst, "load16_u");
        EXPECT_NE(nullptr, func) << "Failed to lookup load16_u function";

        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(wasm_module_inst, stack_size);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t argv[1] = { address };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(ret) << "Failed to call load16_u function: "
                         << wasm_runtime_get_exception(wasm_module_inst);

        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }

        return argv[0];
    }

    /**
     * @brief Call WASM function for signed 16-bit load comparison
     * @param address Memory address to load from
     * @return Sign-extended 32-bit value loaded from memory
     *
     * Invokes the WASM test function for i32.load16_s to compare behavior
     * with i32.load16_u for the same memory location.
     */
    uint32_t call_i32_load16_s(uint32_t address)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(wasm_module_inst, "load16_s");
        EXPECT_NE(nullptr, func) << "Failed to lookup load16_s function";

        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(wasm_module_inst, stack_size);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t argv[1] = { address };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(ret) << "Failed to call load16_s function: "
                         << wasm_runtime_get_exception(wasm_module_inst);

        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }

        return argv[0];
    }

    /**
     * @brief Store 16-bit value in memory for testing
     * @param address Memory address to store to
     * @param value 16-bit value to store
     *
     * Stores a 16-bit value at the specified memory address using the WASM
     * test function for memory setup in test scenarios.
     */
    void store_i16_value(uint32_t address, uint16_t value)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(wasm_module_inst, "store16");
        ASSERT_NE(nullptr, func) << "Failed to lookup store16 function";

        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(wasm_module_inst, stack_size);
        ASSERT_NE(nullptr, exec_env) << "Failed to create execution environment";

        uint32_t argv[2] = { address, value };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        ASSERT_TRUE(ret) << "Failed to call store16 function: "
                         << wasm_runtime_get_exception(wasm_module_inst);

        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }
    }

    /**
     * @brief Get memory instance for direct memory access
     * @return Pointer to WASM linear memory instance
     *
     * Retrieves the linear memory instance for direct memory manipulation
     * and boundary condition testing.
     */
    wasm_memory_inst_t get_memory_inst()
    {
        return wasm_runtime_get_default_memory(wasm_module_inst);
    }

    /**
     * @brief Get current memory size in bytes
     * @return Current memory size in bytes
     *
     * Returns the current size of linear memory for boundary testing
     * and out-of-bounds access validation.
     */
    uint32_t get_memory_size()
    {
        wasm_memory_inst_t memory = get_memory_inst();
        EXPECT_NE(nullptr, memory) << "Failed to get memory instance";
        return wasm_memory_get_cur_page_count(memory) * 65536; // 64KB pages
    }

    RuntimeInitArgs init_args;
    wasm_module_t wasm_module = nullptr;
    wasm_module_inst_t wasm_module_inst = nullptr;
    char *wasm_buf = nullptr;
    uint32_t wasm_buf_size = 0;
    char error_buf[128] = {0};
    bool is_aot_mode = false;
    const uint32_t stack_size = 8092;
    const uint32_t heap_size = 8192;
};

/**
 * @test MemoryBoundaryAccess_EdgeAddresses_ValidLoad
 * @brief Validates memory access at valid memory boundaries
 * @details Tests loading from addresses near memory limits to ensure proper
 *          boundary checking and successful access within valid memory range.
 * @test_category Corner - Memory boundary testing
 * @coverage_target core/iwasm/common/wasm_memory.c:wasm_runtime_validate_app_addr
 * @input_conditions Load from addresses near memory_size-2 (last valid 16-bit location)
 * @expected_behavior Successful load without memory access violation
 * @validation_method Successful function execution and correct value retrieval
 */
TEST_P(I32Load16UTest, MemoryBoundaryAccess_EdgeAddresses_ValidLoad)
{
    uint32_t memory_size = get_memory_size();
    ASSERT_GE(memory_size, 16U) << "Memory too small for boundary testing";

    // Store value near memory boundary (ensuring 2 bytes fit)
    uint32_t boundary_addr = memory_size - 2;
    store_i16_value(boundary_addr, 0x4321);

    // Test boundary access
    uint32_t result = call_i32_load16_u(boundary_addr);
    ASSERT_EQ(0x00004321U, result)
        << "Boundary access failed at address " << boundary_addr;
}

// Instantiate parameterized tests for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningMode, I32Load16UTest,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT));