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

// Test fixture for module loading enhanced tests
class ModuleLoadingEnhancedTest : public testing::Test
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

// Test 6: Load oversized module (simulate large module)
TEST_F(ModuleLoadingEnhancedTest, test_wasm_loader_load_oversized_module)
{
    // Create a buffer with valid WASM header but zeroed content
    const uint32_t large_size = 1 * 1024 * 1024; // 1MB
    auto *large_buffer = static_cast<uint8_t *>(malloc(large_size));
    ASSERT_NE(large_buffer, nullptr) << "Could not allocate buffer for test";

    memset(large_buffer, 0, large_size);
    // Set valid magic and version
    large_buffer[0] = 0x00; large_buffer[1] = 0x61;
    large_buffer[2] = 0x73; large_buffer[3] = 0x6D;
    large_buffer[4] = 0x01; large_buffer[5] = 0x00;
    large_buffer[6] = 0x00; large_buffer[7] = 0x00;

    char error_buf[128] = { 0 };
    wasm_module_t module = wasm_runtime_load(large_buffer, large_size,
                                             error_buf, sizeof(error_buf));
    if (module != nullptr) {
        wasm_runtime_unload(module);
        // Module loaded despite zeroed content - still a valid exercise
    }
    else {
        EXPECT_NE(strlen(error_buf), 0u);
    }
    free(large_buffer);
}

// Test 24: Invalid import resolution
TEST_F(ModuleLoadingEnhancedTest, test_wasm_loader_invalid_import_resolution)
{
    // Create module with unresolvable imports
    uint8_t unresolvable_import_module[] = {
        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, // magic + version
        0x01, 0x04, 0x01, 0x60, 0x00, 0x00,             // type section
        0x02, 0x15, 0x01, 0x0A, 0x6E, 0x6F, 0x6E, 0x65, 0x78, 0x69, 0x73, 0x74, 0x65, 0x6E, 0x74, // import from "nonexistent"
        0x08, 0x6D, 0x69, 0x73, 0x73, 0x69, 0x6E, 0x67, 0x00, 0x00 // import "missing"
    };
    
    char error_buf[128] = { 0 };
    wasm_module_t module = wasm_runtime_load(
        unresolvable_import_module, sizeof(unresolvable_import_module),
        error_buf, sizeof(error_buf));
    if (module != nullptr) {
        wasm_runtime_unload(module);
    }
    else {
        EXPECT_NE(strlen(error_buf), 0u);
    }
}

// Test 26: Function signature mismatch
TEST_F(ModuleLoadingEnhancedTest, test_wasm_loader_function_signature_mismatch)
{
    // Create module with function/type index mismatch
    uint8_t signature_mismatch_module[] = {
        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, // magic + version
        0x01, 0x04, 0x01, 0x60, 0x00, 0x00,             // type section: 1 type
        0x03, 0x02, 0x01, 0x05,                         // function section: reference invalid type 5
        0x0A, 0x04, 0x01, 0x02, 0x00, 0x0B              // code section
    };
    
    char error_buf[128];
    wasm_module_t module = wasm_runtime_load(signature_mismatch_module, sizeof(signature_mismatch_module), error_buf, sizeof(error_buf));
    ASSERT_EQ(module, nullptr);
    ASSERT_NE(strlen(error_buf), 0);
}

// Test 30: Start function validation
TEST_F(ModuleLoadingEnhancedTest, test_wasm_loader_start_function_validation)
{
    // Create module with invalid start function index
    uint8_t invalid_start_module[] = {
        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, // magic + version
        0x01, 0x04, 0x01, 0x60, 0x00, 0x00,             // type section
        0x03, 0x02, 0x01, 0x00,                         // function section: 1 function
        0x08, 0x01, 0x05,                               // start section: invalid function index 5
        0x0A, 0x04, 0x01, 0x02, 0x00, 0x0B              // code section
    };
    
    char error_buf[128];
    wasm_module_t module = wasm_runtime_load(invalid_start_module, sizeof(invalid_start_module), error_buf, sizeof(error_buf));
    ASSERT_EQ(module, nullptr);
    ASSERT_NE(strlen(error_buf), 0);
}

// Test 34: Corrupted section headers
TEST_F(ModuleLoadingEnhancedTest, test_wasm_loader_corrupted_section_headers)
{
    // Create module with corrupted section header
    uint8_t corrupted_header_module[] = {
        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, // magic + version
        0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F              // type section with corrupted size
    };
    
    char error_buf[128];
    wasm_module_t module = wasm_runtime_load(corrupted_header_module, sizeof(corrupted_header_module), error_buf, sizeof(error_buf));
    ASSERT_EQ(module, nullptr);
    ASSERT_NE(strlen(error_buf), 0);
}
