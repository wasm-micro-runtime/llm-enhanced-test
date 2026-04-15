/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <climits>
#include "gtest/gtest.h"
#include "wasm_runtime_common.h"
#include "wasm_loader.h"
#include "bh_platform.h"
#include "../common/test_helper.h"

// Test fixture for instance creation enhanced tests
class InstanceCreationEnhancedTest : public testing::Test
{
protected:
    virtual void SetUp()
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
        ASSERT_EQ(wasm_runtime_full_init(&init_args), true);
    }

    virtual void TearDown() 
    { 
        wasm_runtime_destroy(); 
    }

    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

// Test 14: Instance memory access validation
TEST_F(InstanceCreationEnhancedTest, test_wasm_runtime_instance_memory_access)
{
    WAMRModule module(dummy_wasm_buffer, sizeof(dummy_wasm_buffer));
    ASSERT_NE(module.get(), nullptr);
    
    wasm_module_inst_t module_inst = wasm_runtime_instantiate(module.get(), 8192, 8192, nullptr, 0);
    ASSERT_NE(module_inst, nullptr);
    
    // Test memory access functions
    wasm_memory_inst_t memory = wasm_runtime_get_default_memory(module_inst);
    EXPECT_NE(memory, nullptr);
    
    if (memory != nullptr) {
        uint64_t page_count = wasm_memory_get_cur_page_count(memory);
        uint64_t bytes_per_page = wasm_memory_get_bytes_per_page(memory);
        EXPECT_GT(page_count, 0);
        EXPECT_GT(bytes_per_page, 0);
    }
    
    wasm_runtime_deinstantiate(module_inst);
}
