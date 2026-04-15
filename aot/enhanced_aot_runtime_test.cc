/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <climits>
#include "gtest/gtest.h"
#include "wasm_export.h"
#include "aot_runtime.h"
#include "aot.h"
#include "bh_bitmap.h"

// Enhanced test fixture for aot_runtime.c functions
class EnhancedAotRuntimeTest : public testing::Test {
protected:
    void SetUp() override {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
    }

    void TearDown() override {
        wasm_runtime_destroy();
    }

public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

/******
 * Test Case: aot_resolve_symbols_MixedLinkedUnlinked_PartialFailure
 * Source: core/iwasm/aot/aot_runtime.c:5525-5531
 * Target Lines: 5525-5531 (complete iteration with mixed success/failure)
 * Functional Purpose: Validates that aot_resolve_symbols() correctly processes
 *                     modules with mixed linked/unlinked functions and returns false
 *                     when any unlinked function fails resolution.
 * Call Path: aot_resolve_symbols() <- wasm_runtime_resolve_symbols() <- public API
 * Coverage Goal: Exercise complete iteration logic with partial failures
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_resolve_symbols_MixedLinkedUnlinked_PartialFailure) {
    // Create a minimal AOT module with mixed import functions
    AOTModule test_module;
    memset(&test_module, 0, sizeof(AOTModule));

    // Create array of import functions
    AOTImportFunc import_funcs[3];
    memset(import_funcs, 0, sizeof(import_funcs));

    // Set up first import function (already linked - should be skipped)
    import_funcs[0].module_name = (char*)"linked_module";
    import_funcs[0].func_name = (char*)"linked_function";
    import_funcs[0].func_ptr_linked = (void*)0xABCDEF12; // Already linked

    // Create minimal function type for first function
    AOTFuncType func_type1;
    memset(&func_type1, 0, sizeof(AOTFuncType));
    func_type1.param_count = 0;
    func_type1.result_count = 0;
    import_funcs[0].func_type = &func_type1;

    // Set up second import function (unlinked - will fail)
    import_funcs[1].module_name = (char*)"fail_module1";
    import_funcs[1].func_name = (char*)"fail_function1";
    import_funcs[1].func_ptr_linked = nullptr; // Not linked

    // Create minimal function type for second function
    AOTFuncType func_type2;
    memset(&func_type2, 0, sizeof(AOTFuncType));
    func_type2.param_count = 0;
    func_type2.result_count = 0;
    import_funcs[1].func_type = &func_type2;

    // Set up third import function (unlinked - will also fail)
    import_funcs[2].module_name = (char*)"fail_module2";
    import_funcs[2].func_name = (char*)"fail_function2";
    import_funcs[2].func_ptr_linked = nullptr; // Not linked

    // Create minimal function type for third function
    AOTFuncType func_type3;
    memset(&func_type3, 0, sizeof(AOTFuncType));
    func_type3.param_count = 0;
    func_type3.result_count = 0;
    import_funcs[2].func_type = &func_type3;

    // Configure module with import functions
    test_module.import_funcs = import_funcs;
    test_module.import_func_count = 3;

    // Test the function - should process all three functions
    bool result = aot_resolve_symbols(&test_module);

    // Should return false due to failed resolutions
    ASSERT_FALSE(result);

    // First function should remain linked
    ASSERT_EQ(import_funcs[0].func_ptr_linked, (void*)0xABCDEF12);

    // Second and third functions should still be unlinked
    ASSERT_EQ(import_funcs[1].func_ptr_linked, nullptr);
    ASSERT_EQ(import_funcs[2].func_ptr_linked, nullptr);
}

/******
 * Test Case: aot_const_str_set_insert_DuplicateString_ReturnsExistingString
 * Source: core/iwasm/aot/aot_runtime.c:5431-5476
 * Target Lines: 5464-5467 (hash map lookup and early return)
 * Functional Purpose: Validates that aot_const_str_set_insert() correctly finds
 *                     existing strings in the hash map and returns them without
 *                     creating duplicates.
 * Call Path: Direct call to aot_const_str_set_insert() with existing string
 * Coverage Goal: Exercise string deduplication logic
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_const_str_set_insert_DuplicateString_ReturnsExistingString) {
    // Create a minimal AOT module for testing
    AOTModule test_module;
    memset(&test_module, 0, sizeof(AOTModule));
    test_module.const_str_set = nullptr;

    // Test string data
    const char* test_string = "duplicate_function_name";
    int32 str_len = (int32)(strlen(test_string) + 1);
    char error_buf[256];

    // First insertion - should create new entry
    char* first_result = aot_const_str_set_insert((const uint8*)test_string, str_len, &test_module,
#if (WASM_ENABLE_WORD_ALIGN_READ != 0)
                                                 false,
#endif
                                                 error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, first_result);

    // Second insertion of same string - should return existing entry
    char* second_result = aot_const_str_set_insert((const uint8*)test_string, str_len, &test_module,
#if (WASM_ENABLE_WORD_ALIGN_READ != 0)
                                                  false,
#endif
                                                  error_buf, sizeof(error_buf));

    // Verify same pointer is returned (deduplication)
    ASSERT_EQ(first_result, second_result);
    ASSERT_STREQ(test_string, second_result);

    // Cleanup
    if (test_module.const_str_set != nullptr) {
        bh_hash_map_destroy(test_module.const_str_set);
    }
}

#if (WASM_ENABLE_WORD_ALIGN_READ != 0)
#endif

/******
 * Test Case: aot_memory_init_InvalidAppAddr_ValidationFailure
 * Source: core/iwasm/aot/aot_runtime.c:3539-3579
 * Target Lines: 3562-3564 (application address validation failure)
 * Functional Purpose: Tests the address validation path where wasm_runtime_validate_app_addr
 *                     fails due to invalid destination address, ensuring proper error handling
 *                     in bulk memory operations.
 * Call Path: aot_memory_init() <- AOT compiled code <- WebAssembly bulk memory operations
 * Coverage Goal: Exercise address validation failure path for error handling
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_memory_init_InvalidAppAddr_ValidationFailure) {
    // Create AOT module instance with invalid destination address
    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    AOTMemoryInstance memory_inst;
    AOTModule aot_module;
    AOTMemInitData mem_init_data;
    AOTMemInitData *mem_init_data_list[1];

    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));
    memset(&memory_inst, 0, sizeof(AOTMemoryInstance));
    memset(&aot_module, 0, sizeof(AOTModule));
    memset(&mem_init_data, 0, sizeof(AOTMemInitData));

    // Setup module instance structure
    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.module = (WASMModule*)&aot_module;
    module_inst.memory_count = 1;
    // Allocate array of memory instance pointers
    module_inst.memories = (WASMMemoryInstance**)wasm_runtime_malloc(sizeof(WASMMemoryInstance*));
    ASSERT_NE(nullptr, module_inst.memories);
    module_inst.memories[0] = (WASMMemoryInstance*)&memory_inst;

    // Setup memory instance with small memory size
    memory_inst.memory_data_size = 1024; // Small memory size
    memory_inst.memory_data = (uint8*)wasm_runtime_malloc(memory_inst.memory_data_size);
    ASSERT_NE(nullptr, memory_inst.memory_data);

    // Setup valid memory initialization data
    const char test_data[] = "Test data";
    mem_init_data.byte_count = strlen(test_data);
    mem_init_data.bytes = (uint8*)test_data;
    mem_init_data_list[0] = &mem_init_data;

    aot_module.mem_init_data_count = 1;
    aot_module.mem_init_data_list = mem_init_data_list;

    // Initialize data_dropped bitmap (not dropped)
    extra.common.data_dropped = bh_bitmap_new(0, 1);
    ASSERT_NE(nullptr, extra.common.data_dropped);

    // Test parameters with invalid destination address (beyond memory bounds)
    uint32 seg_index = 0;
    uint32 offset = 0;
    uint32 len = strlen(test_data);
    size_t dst = memory_inst.memory_data_size + 1000; // Invalid destination beyond memory

    // Execute aot_memory_init
    bool result = aot_memory_init(&module_inst, seg_index, offset, len, dst);

    // Assert validation failure (wasm_runtime_validate_app_addr fails)
    ASSERT_FALSE(result);

    // Cleanup
    wasm_runtime_free(memory_inst.memory_data);
    wasm_runtime_free(module_inst.memories);
    bh_bitmap_delete(extra.common.data_dropped);
}

/******
 * Test Case: aot_memory_init_OutOfBounds_ExceptionSet
 * Source: core/iwasm/aot/aot_runtime.c:3539-3579
 * Target Lines: 3566-3569 (bounds checking and exception setting)
 * Functional Purpose: Tests the bounds checking path where offset + len exceeds segment length,
 *                     ensuring proper exception setting via aot_set_exception for out of
 *                     bounds memory access in bulk memory operations.
 * Call Path: aot_memory_init() <- AOT compiled code <- WebAssembly bulk memory operations
 * Coverage Goal: Exercise bounds violation exception handling path
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_memory_init_OutOfBounds_ExceptionSet) {
    // Create AOT module instance with bounds violation scenario
    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    AOTMemoryInstance memory_inst;
    AOTModule aot_module;
    AOTMemInitData mem_init_data;
    AOTMemInitData *mem_init_data_list[1];

    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));
    memset(&memory_inst, 0, sizeof(AOTMemoryInstance));
    memset(&aot_module, 0, sizeof(AOTModule));
    memset(&mem_init_data, 0, sizeof(AOTMemInitData));

    // Setup module instance structure
    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.module = (WASMModule*)&aot_module;
    module_inst.memory_count = 1;
    // Allocate array of memory instance pointers
    module_inst.memories = (WASMMemoryInstance**)wasm_runtime_malloc(sizeof(WASMMemoryInstance*));
    ASSERT_NE(nullptr, module_inst.memories);
    module_inst.memories[0] = (WASMMemoryInstance*)&memory_inst;

    // Setup memory instance
    memory_inst.memory_data_size = 65536;
    memory_inst.memory_data = (uint8*)wasm_runtime_malloc(memory_inst.memory_data_size);
    ASSERT_NE(nullptr, memory_inst.memory_data);

    // Setup memory initialization data with small segment
    const char test_data[] = "Small";
    mem_init_data.byte_count = strlen(test_data); // Only 5 bytes available
    mem_init_data.bytes = (uint8*)test_data;
    mem_init_data_list[0] = &mem_init_data;

    aot_module.mem_init_data_count = 1;
    aot_module.mem_init_data_list = mem_init_data_list;

    // Initialize data_dropped bitmap (not dropped)
    extra.common.data_dropped = bh_bitmap_new(0, 1);
    ASSERT_NE(nullptr, extra.common.data_dropped);

    // Test parameters with out of bounds access (offset + len > seg_len)
    uint32 seg_index = 0;
    uint32 offset = 3;  // Start at offset 3 in 5-byte segment
    uint32 len = 5;     // Try to read 5 bytes, but only 2 bytes available (5-3=2)
    size_t dst = 1024;  // Valid destination

    // Execute aot_memory_init
    bool result = aot_memory_init(&module_inst, seg_index, offset, len, dst);

    // Assert out of bounds exception (offset + len > seg_len)
    ASSERT_FALSE(result);

    // Cleanup
    wasm_runtime_free(memory_inst.memory_data);
    wasm_runtime_free(module_inst.memories);
    bh_bitmap_delete(extra.common.data_dropped);
}
