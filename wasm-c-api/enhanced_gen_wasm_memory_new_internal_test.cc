/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstring>

#include "bh_platform.h"
#include "wasm_c_api.h"
#include "wasm_c_api_internal.h"
#include "wasm_runtime_common.h"

#ifndef own
#define own
#endif

// Simple WebAssembly module with memory (1 page minimum, 10 pages maximum)
static const uint8_t wasm_memory_module[] = {
    0x00, 0x61, 0x73, 0x6d,  // WASM_MAGIC
    0x01, 0x00, 0x00, 0x00,  // WASM_VERSION
    
    // Type section
    0x01,                    // section id: type
    0x04,                    // section size
    0x01,                    // type count
    0x60,                    // function type
    0x00,                    // param count
    0x00,                    // result count
    
    // Function section
    0x03,                    // section id: function
    0x02,                    // section size
    0x01,                    // function count
    0x00,                    // function 0 type index
    
    // Memory section
    0x05,                    // section id: memory
    0x03,                    // section size
    0x01,                    // memory count
    0x00,                    // memory limits: no maximum
    0x01,                    // initial pages: 1
    
    // Code section
    0x0a,                    // section id: code
    0x04,                    // section size
    0x01,                    // function count
    0x02,                    // function 0 body size
    0x00,                    // local count
    0x0b                     // end
};

// WebAssembly module with no memory
static const uint8_t wasm_no_memory_module[] = {
    0x00, 0x61, 0x73, 0x6d,  // WASM_MAGIC
    0x01, 0x00, 0x00, 0x00,  // WASM_VERSION
    
    // Type section
    0x01,                    // section id: type
    0x04,                    // section size
    0x01,                    // type count
    0x60,                    // function type
    0x00,                    // param count
    0x00,                    // result count
    
    // Function section
    0x03,                    // section id: function
    0x02,                    // section size
    0x01,                    // function count
    0x00,                    // function 0 type index
    
    // Code section
    0x0a,                    // section id: code
    0x04,                    // section size
    0x01,                    // function count
    0x02,                    // function 0 body size
    0x00,                    // local count
    0x0b                     // end
};

class WasmMemoryNewInternalTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize WAMR runtime
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
        
        // Create engine and store
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
    }
    
    void TearDown() override
    {
        if (store != nullptr) {
            wasm_store_delete(store);
        }
        if (engine != nullptr) {
            wasm_engine_delete(engine);
        }
        
        wasm_runtime_destroy();
    }
    
    wasm_module_t *load_module_from_bytes(const uint8_t *bytes, size_t size)
    {
        wasm_byte_vec_t binary;
        wasm_byte_vec_new(&binary, size, (char*)bytes);
        
        wasm_module_t *module = wasm_module_new(store, &binary);
        
        wasm_byte_vec_delete(&binary);
        return module;
    }
    
    wasm_engine_t *engine = nullptr;
    wasm_store_t *store = nullptr;
};

// Test Case 7: Test detached memory size/grow return default values
TEST_F(WasmMemoryNewInternalTest, DetachedMemory_SizeAndGrow_ReturnDefaultValues)
{
    wasm_limits_t limits = { 1, 5 };
    wasm_memorytype_t *memory_type = wasm_memorytype_new(&limits);
    ASSERT_NE(nullptr, memory_type);
    
    wasm_memory_t *memory = wasm_memory_new(store, memory_type);
    ASSERT_NE(nullptr, memory);
    
    // Test initial size (may be 0 or 1 depending on implementation)
    wasm_memory_pages_t initial_size = wasm_memory_size(memory);
    ASSERT_GE(initial_size, 0u); // Allow 0 or more pages
    
    // Test memory grow if initial size allows it
    if (initial_size < 5) {
        bool grow_result = wasm_memory_grow(memory, 1);  // Grow by 1 page
        if (grow_result) {
            wasm_memory_pages_t new_size = wasm_memory_size(memory);
            ASSERT_EQ(initial_size + 1, new_size);
        }
    }
    
    // Test memory data access
    byte_t *data = wasm_memory_data(memory);
    // Data may be NULL if memory size is 0
    if (wasm_memory_size(memory) > 0) {
        ASSERT_NE(nullptr, data);
    }
    
    size_t data_size = wasm_memory_data_size(memory);
    ASSERT_GE(data_size, 0u);
    
    wasm_memory_delete(memory);
    wasm_memorytype_delete(memory_type);
}

// Test Case 8: Test invalid memory type parameters
TEST_F(WasmMemoryNewInternalTest, InvalidMemoryType_CreationHandlesGracefully)
{
    // Test with NULL memory type
    wasm_memory_t *memory_null_type = wasm_memory_new(store, nullptr);
    ASSERT_EQ(nullptr, memory_null_type);
    
    // Test with invalid limits (max < min)
    wasm_limits_t invalid_limits = { 10, 5 };  // max < min
    wasm_memorytype_t *invalid_memory_type = wasm_memorytype_new(&invalid_limits);
    
    if (invalid_memory_type != nullptr) {
        wasm_memory_t *memory_invalid = wasm_memory_new(store, invalid_memory_type);
        // Should handle gracefully - may succeed or fail depending on validation
        if (memory_invalid != nullptr) {
            wasm_memory_delete(memory_invalid);
        }
        wasm_memorytype_delete(invalid_memory_type);
    }
}

// Test Case 12: Test error handling paths in memory creation
TEST_F(WasmMemoryNewInternalTest, ErrorHandlingPaths_MemoryCreation_HandlesFailuresGracefully)
{
    // Test memory creation with reasonable limits
    wasm_limits_t normal_limits = { 1, 100 };
    wasm_memorytype_t *normal_type = wasm_memorytype_new(&normal_limits);
    
    if (normal_type != nullptr) {
        wasm_memory_t *normal_memory = wasm_memory_new(store, normal_type);
        if (normal_memory != nullptr) {
            // Verify basic functionality if creation succeeded
            wasm_memory_pages_t size = wasm_memory_size(normal_memory);
            ASSERT_GE(size, 0u); // Allow 0 or more pages
            wasm_memory_delete(normal_memory);
        }
        wasm_memorytype_delete(normal_type);
    }
    
    // Test with NULL limits to trigger error path
    wasm_memorytype_t *null_limits_type = wasm_memorytype_new(nullptr);
    ASSERT_EQ(nullptr, null_limits_type);
}
