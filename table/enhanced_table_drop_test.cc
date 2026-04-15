/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "bh_read_file.h"
#include "wasm_runtime_common.h"
#include <cstring>
#include <climits>

/**
 * @brief Test fixture class for table.drop opcode validation
 * @details Provides comprehensive testing environment for table.drop operations including
 *          proper WAMR runtime initialization, module loading, and cleanup with RAII patterns
 */
class TableDropTest : public testing::TestWithParam<RunningMode> {
protected:
    static constexpr const char* WASM_FILE = "wasm-apps/table_drop_test.wasm";
    static constexpr uint32_t DEFAULT_STACK_SIZE = 16 * 1024;
    static constexpr uint32_t DEFAULT_HEAP_SIZE = 16 * 1024;

    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    char error_buf[128] = { 0 };
    uint8 *wasm_file_buf = nullptr;
    uint32 wasm_file_size = 0;

    /**
     * @brief Initialize WAMR runtime and load test module
     * @details Sets up complete WAMR environment with interpreter/AOT mode based on test parameters
     */
    void SetUp() override {
        memset(error_buf, 0, sizeof(error_buf));

        // Initialize WAMR runtime with proper configuration
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        init_args.mem_alloc_option.allocator.malloc_func = (void*)malloc;
        init_args.mem_alloc_option.allocator.realloc_func = (void*)realloc;
        init_args.mem_alloc_option.allocator.free_func = (void*)free;

        // Configure based on test execution mode parameter
        RunningMode running_mode = GetParam();
        if (running_mode == Mode_Interp) {
            init_args.running_mode = Mode_Interp;
        }
        else {
            init_args.running_mode = Mode_LLVM_JIT;
        }

        ASSERT_TRUE(wasm_runtime_full_init(&init_args)) << "Failed to initialize WAMR runtime";

        // Load WASM test module from file system
        wasm_file_buf = (uint8_t*)bh_read_file_to_buffer(WASM_FILE, &wasm_file_size);
        ASSERT_NE(nullptr, wasm_file_buf) << "Failed to read WASM file: " << WASM_FILE;
        ASSERT_GT(wasm_file_size, 0U) << "WASM file size must be greater than zero";

        // Load and validate WASM module structure
        module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        // Instantiate module with sufficient stack and heap
        module_inst = wasm_runtime_instantiate(module, DEFAULT_STACK_SIZE, DEFAULT_HEAP_SIZE,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate WASM module: " << error_buf;

        // Create execution environment for function calls
        exec_env = wasm_runtime_create_exec_env(module_inst, DEFAULT_STACK_SIZE);
        ASSERT_NE(nullptr, exec_env) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up WAMR resources with proper RAII pattern
     * @details Ensures all allocated resources are properly freed to prevent memory leaks
     */
    void TearDown() override {
        // Clean up execution environment
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
            exec_env = nullptr;
        }

        // Clean up module instance
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }

        // Clean up loaded module
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }

        // Free file buffer
        if (wasm_file_buf != nullptr) {
            BH_FREE(wasm_file_buf);
            wasm_file_buf = nullptr;
        }

        // Destroy WAMR runtime
        wasm_runtime_destroy();
    }

    /**
     * @brief Call WASM function to drop element segment by index
     * @param segment_index Element segment index to drop
     * @return true if drop operation succeeded, false otherwise
     */
    bool call_table_drop(uint32_t segment_index) {
        // Look up table.drop test function
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_table_drop");
        EXPECT_NE(nullptr, func) << "Failed to lookup test_table_drop function";
        if (func == nullptr) return false;

        // Prepare function arguments
        wasm_val_t args[1];
        args[0].kind = WASM_I32;
        args[0].of.i32 = static_cast<int32_t>(segment_index);

        wasm_val_t results[1];
        results[0].kind = WASM_I32;

        // Execute function and capture result
        bool call_success = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, args);
        if (!call_success) {
            const char* exception = wasm_runtime_get_exception(module_inst);
            printf("WASM call failed: %s\n", exception != nullptr ? exception : "Unknown error");
            return false;
        }

        return results[0].of.i32 == 1;  // 1 indicates successful drop
    }

    /**
     * @brief Call WASM function to test table.init with specified parameters
     * @param elem_index Element segment index
     * @param dest Destination table offset
     * @param src Source element offset
     * @param len Number of elements to copy
     * @return true if table.init succeeded, false if failed (expected for dropped segments)
     */
    bool call_table_init(uint32_t elem_index, uint32_t dest, uint32_t src, uint32_t len) {
        // Look up table.init test function
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_table_init");
        EXPECT_NE(nullptr, func) << "Failed to lookup test_table_init function";
        if (func == nullptr) return false;

        // Prepare function arguments
        wasm_val_t args[4];
        args[0].kind = WASM_I32; args[0].of.i32 = static_cast<int32_t>(elem_index);
        args[1].kind = WASM_I32; args[1].of.i32 = static_cast<int32_t>(dest);
        args[2].kind = WASM_I32; args[2].of.i32 = static_cast<int32_t>(src);
        args[3].kind = WASM_I32; args[3].of.i32 = static_cast<int32_t>(len);

        wasm_val_t results[1];
        results[0].kind = WASM_I32;

        // Execute function and capture result
        bool call_success = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 4, args);
        if (!call_success) {
            const char* exception = wasm_runtime_get_exception(module_inst);
            printf("WASM table.init call failed: %s\n", exception != nullptr ? exception : "Unknown error");
            return false;  // Function call failed - likely due to trap
        }

        return results[0].of.i32 == 1;  // 1 indicates successful init
    }

    /**
     * @brief Call WASM function to test invalid table.drop operations (expected to fail)
     * @param segment_index Invalid element segment index
     * @return true if function executed (not trapped), false if trapped as expected
     */
    bool call_invalid_table_drop(uint32_t segment_index) {
        // Look up invalid table.drop test function
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_invalid_table_drop");
        EXPECT_NE(nullptr, func) << "Failed to lookup test_invalid_table_drop function";
        if (func == nullptr) return false;

        // Prepare function arguments
        wasm_val_t args[1];
        args[0].kind = WASM_I32;
        args[0].of.i32 = static_cast<int32_t>(segment_index);

        wasm_val_t results[1];
        results[0].kind = WASM_I32;

        // Execute function - should fail for invalid indices
        bool call_success = wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, args);

        // For invalid operations, we expect the call to fail (trap)
        return !call_success;  // Return true if trapped as expected
    }
};

/**
 * @test BoundaryIndices_HandledCorrectly
 * @brief Validates table.drop behavior with boundary element segment indices
 * @details Tests edge cases for element segment indices including minimum (0) and maximum
 *          valid indices to ensure proper boundary condition handling.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:table_drop_validation
 * @input_conditions Element indices: 0 (minimum), 2 (maximum valid for test module)
 * @expected_behavior Successful execution for all valid boundary indices
 * @validation_method Boundary value testing with success confirmation
 */
TEST_P(TableDropTest, BoundaryIndices_HandledCorrectly) {
    // Test minimum valid index (0)
    ASSERT_TRUE(call_table_drop(0)) << "Failed to drop element segment at minimum index 0";

    // Test maximum valid index (2 for our test module with 3 segments)
    ASSERT_TRUE(call_table_drop(2)) << "Failed to drop element segment at maximum valid index 2";

    // Verify both dropped segments prevent table.init
    ASSERT_FALSE(call_table_init(0, 0, 0, 1))
        << "table.init should fail for dropped boundary segment 0";
    ASSERT_FALSE(call_table_init(2, 0, 0, 1))
        << "table.init should fail for dropped boundary segment 2";
}

// Test parameter instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningMode, TableDropTest,
                        testing::Values(Mode_Interp
#if WASM_ENABLE_AOT != 0
                                      , Mode_LLVM_JIT
#endif
                        ));
