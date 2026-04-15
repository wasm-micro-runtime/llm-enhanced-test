/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include "wasm_runtime_common.h"
#include "bh_read_file.h"
#include "test_helper.h"

/**
 * @brief Enhanced unit tests for WASM memory.grow opcode
 *
 * Tests comprehensive functionality of the memory.grow instruction which attempts
 * to grow linear memory by specified pages and returns previous size or -1 on failure.
 * Validates behavior across interpreter and AOT execution modes with various memory
 * configurations including boundary conditions, failure scenarios, and edge cases.
 */

/**
 * @brief Base class for memory.grow tests with common functionality
 */
class MemoryGrowTestBase : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Set up test environment with WASM module loading and runtime initialization
     */
    void SetUp() override {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Initialize members
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;
        stack_size = 65536;
        heap_size = 0;  // No additional heap to avoid affecting memory operations

        // Get current working directory for WASM file loading
        char *cwd_ptr = getcwd(nullptr, 0);
        ASSERT_NE(cwd_ptr, nullptr) << "Failed to get current working directory";
        cwd = std::string(cwd_ptr);
        free(cwd_ptr);

        loadWasmModule();
    }

    /**
     * @brief Clean up test environment and release all resources
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
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM module - to be implemented by derived classes for specific memory configurations
     */
    virtual void loadWasmModule() = 0;

    /**
     * @brief Call WASM function that executes memory.grow instruction
     * @param func_name Name of the exported WASM function to call
     * @param pages_to_grow Number of pages to grow memory by
     * @return Previous memory size in pages, or -1 if growth failed
     */
    int32_t call_memory_grow_function(const char* func_name, uint32_t pages_to_grow) {
        EXPECT_NE(nullptr, module_inst) << "Module instance not initialized";
        EXPECT_NE(nullptr, exec_env) << "Execution environment not initialized";

        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Function not found: " << func_name;
        if (func == nullptr) return -1;

        uint32_t argv[1] = {pages_to_grow};
        bool call_result = wasm_runtime_call_wasm(exec_env, func, 1, argv);
        EXPECT_TRUE(call_result) << "Function call failed: " << wasm_runtime_get_exception(module_inst);
        if (!call_result) return -1;

        return (int32_t)argv[0];
    }

    /**
     * @brief Call WASM function that executes memory.size instruction
     * @param func_name Name of the exported WASM function to call
     * @return Current memory size in pages
     */
    int32_t call_memory_size_function(const char* func_name) {
        EXPECT_NE(nullptr, module_inst) << "Module instance not initialized";
        EXPECT_NE(nullptr, exec_env) << "Execution environment not initialized";

        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(nullptr, func) << "Function not found: " << func_name;
        if (func == nullptr) return -1;

        uint32_t argv[1] = {0};  // memory.size takes no arguments
        bool call_result = wasm_runtime_call_wasm(exec_env, func, 0, argv);
        EXPECT_TRUE(call_result) << "Function call failed: " << wasm_runtime_get_exception(module_inst);
        if (!call_result) return -1;

        return (int32_t)argv[0];
    }

    // Common test members
    RuntimeInitArgs init_args;
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;
    uint32_t buf_size, stack_size, heap_size;
    uint8_t *buf;
    char error_buf[128];
    std::string cwd;
};

/**
 * @brief Test class for basic memory growth scenarios with reasonable limits
 */
class MemoryGrowBasicTest : public MemoryGrowTestBase {
protected:
    /**
     * @brief Load WASM module with basic memory configuration (initial 2 pages, max 10 pages)
     */
    void loadWasmModule() override {
        std::string wasm_file = cwd + "/wasm-apps/memory_grow_test.wasm";
        buf = (uint8_t *)bh_read_file_to_buffer(wasm_file.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << wasm_file;

        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }
};

/**
 * @brief Test class for boundary testing with limited maximum memory (initial 1 page, max 3 pages)
 */
class MemoryGrowLimitedTest : public MemoryGrowTestBase {
protected:
    /**
     * @brief Load WASM module with limited memory configuration for boundary testing
     */
    void loadWasmModule() override {
        std::string wasm_file = cwd + "/wasm-apps/memory_grow_limited_test.wasm";
        buf = (uint8_t *)bh_read_file_to_buffer(wasm_file.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << wasm_file;

        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }
};

/**
 * @test MaximumMemoryGrowth_SucceedsToLimit
 * @brief Validates memory.grow succeeds when growing exactly to maximum memory limit
 * @details Tests successful growth to the exact maximum memory limit, ensuring
 *          proper handling of boundary conditions for successful operations.
 * @test_category Corner - Maximum boundary validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:memory_grow_exact_limit
 * @input_conditions Module max 3 pages, current 1 page, grow by 2 pages (reaches max)
 * @expected_behavior Returns 1 (previous size), new size becomes 3 pages (maximum)
 * @validation_method Success return value validation and exact limit verification
 */
TEST_P(MemoryGrowLimitedTest, MaximumMemoryGrowth_SucceedsToLimit) {
    // Verify initial state
    int32_t initial_size = call_memory_size_function("get_memory_size");
    ASSERT_EQ(1, initial_size) << "Expected 1 page initial memory";

    // Grow to exact maximum (1 + 2 = 3 pages, which is the maximum)
    int32_t grow_result = call_memory_grow_function("grow_memory", 2);
    ASSERT_EQ(1, grow_result) << "Expected previous size 1 when growing to limit";

    // Verify we reached exactly the maximum
    int32_t size_at_limit = call_memory_size_function("get_memory_size");
    ASSERT_EQ(3, size_at_limit) << "Expected 3 pages (maximum) after growth";

    // Verify further growth fails
    int32_t further_growth = call_memory_grow_function("grow_memory", 1);
    ASSERT_EQ(-1, further_growth) << "Expected failure when already at maximum";
}

/**
 * @test ExcessiveGrowth_ReturnsFailure
 * @brief Validates consistent failure handling for various impossible growth scenarios
 * @details Tests multiple scenarios where memory growth should fail, ensuring
 *          consistent -1 return value and proper memory state preservation.
 * @test_category Error - Failure scenario validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:memory_grow_failures
 * @input_conditions Various impossible growth scenarios beyond system/module limits
 * @expected_behavior Consistent -1 return for all failure cases, no memory state changes
 * @validation_method Comprehensive failure pattern verification and state consistency validation
 */
TEST_P(MemoryGrowLimitedTest, ExcessiveGrowth_ReturnsFailure) {
    // Record initial state
    int32_t initial_size = call_memory_size_function("get_memory_size");
    ASSERT_EQ(1, initial_size) << "Expected 1 page initial memory for limited module";

    // Test various excessive growth scenarios
    std::vector<uint32_t> excessive_values = {100, 1000, 10000, 0xFFFFFFFF};

    for (uint32_t excessive_growth : excessive_values) {
        int32_t grow_result = call_memory_grow_function("grow_memory", excessive_growth);
        ASSERT_EQ(-1, grow_result) << "Expected failure (-1) for excessive growth: " << excessive_growth;

        // Verify memory state unchanged after each failure
        int32_t current_size = call_memory_size_function("get_memory_size");
        ASSERT_EQ(initial_size, current_size)
            << "Memory size changed after failed growth by " << excessive_growth;
    }
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_CASE_P(RunningModeTest, MemoryGrowLimitedTest,
                        testing::Values(RunningMode::Mode_Interp, RunningMode::Mode_LLVM_JIT));
                        