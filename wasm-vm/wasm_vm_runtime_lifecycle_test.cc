/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "bh_platform.h"
#include "wasm_export.h"
#include "../../common/test_helper.h"

// Step 1: VM Core Lifecycle - Runtime Management Test Suite
// Feature Focus: Runtime initialization, configuration, and basic lifecycle management
// Test Categories: Runtime init/destroy, configuration validation, basic resource management

class WasmVMRuntimeLifecycleTest : public testing::Test
{
protected:
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        memset(global_heap_buf, 0, sizeof(global_heap_buf));
        runtime_initialized = false;
    }

    void TearDown() override
    {
        if (runtime_initialized) {
            wasm_runtime_destroy();
            runtime_initialized = false;
        }
    }

    // Helper method to initialize runtime with specific configuration
    bool InitializeRuntime(RuntimeInitArgs *args)
    {
        bool result = wasm_runtime_full_init(args);
        if (result) {
            runtime_initialized = true;
        }
        return result;
    }

    // Helper method to create default init args
    void SetupDefaultInitArgs()
    {
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
    }

    // Safe runtime validation without crashing
    bool ValidateRuntimeState() const
    {
        // Basic validation that runtime is in expected state
        return runtime_initialized;
    }

    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
    bool runtime_initialized;
};

// Test 5: Runtime initialization with zero heap size (should fail)
TEST_F(WasmVMRuntimeLifecycleTest, test_runtime_init_zero_heap_size)
{
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = 0;
    
    bool result = InitializeRuntime(&init_args);
    EXPECT_FALSE(result);
    EXPECT_FALSE(ValidateRuntimeState());
}

// Test 9: Runtime initialization failure scenarios
TEST_F(WasmVMRuntimeLifecycleTest, test_runtime_full_init_failure)
{
    // Test with invalid memory allocation type
    init_args.mem_alloc_type = (mem_alloc_type_t)999; // Invalid type
    
    // Test that it handles invalid input gracefully
    bool result = InitializeRuntime(&init_args);
    EXPECT_FALSE(result);
}

