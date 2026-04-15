/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <climits>
#include "gtest/gtest.h"
#include "wasm_export.h"
#include "bh_platform.h"
#include "aot_llvm.h"
#include "aot_intrinsic.h"
#include "aot.h"

// Enhanced test fixture for coverage improvement
class EnhancedAOTTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
    }

    virtual void TearDown() { wasm_runtime_destroy(); }

  public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

TEST_F(EnhancedAOTTest, wasm_loader_set_error_buf_ValidErrorBuffer_FormatsMessage) {
    // This test targets the formatting path in wasm_loader_set_error_buf during load failure
    // Line 17: snprintf(error_buf, ...)

    uint8_t invalid_aot_data[] = {0x00, 0x61, 0x73, 0x6d}; // Invalid WASM magic
    uint32_t data_size = sizeof(invalid_aot_data);
    char error_buf[256];
    memset(error_buf, 0, sizeof(error_buf));

    // Load module with valid error buffer - should trigger wasm_loader_set_error_buf formatting
    wasm_module_t module = wasm_runtime_load(invalid_aot_data, data_size, error_buf, sizeof(error_buf));
    ASSERT_EQ(nullptr, module);

    // Verify error message was generated during load failure
    ASSERT_GT(strlen(error_buf), 0);
}

TEST_F(EnhancedAOTTest, wasm_loader_set_error_buf_MalformedData_FormatsMessage) {
    // This test targets error message formatting with completely malformed data
    // Tests wasm_loader_set_error_buf with 0xFF-filled invalid data pattern

    // Use a scenario that would generate an error message
    uint8_t malformed_aot_data[1024];
    memset(malformed_aot_data, 0xFF, sizeof(malformed_aot_data)); // Fill with invalid data
    char error_buf[512];
    memset(error_buf, 0, sizeof(error_buf));

    // This should trigger error handling during WASM load failure
    wasm_module_t module = wasm_runtime_load(malformed_aot_data, sizeof(malformed_aot_data), error_buf, sizeof(error_buf));
    ASSERT_EQ(nullptr, module);

    // Verify error message was generated
    ASSERT_GT(strlen(error_buf), 0);
}

TEST_F(EnhancedAOTTest, wasm_loader_set_error_buf_LoadAndInstantiate_ErrorPaths) {
    // This test targets error buffer usage during WASM module load and instantiation
    // Tests wasm_loader_set_error_buf through various error paths

    uint8_t simple_wasm[] = {
        0x00, 0x61, 0x73, 0x6d, // WASM magic
        0x01, 0x00, 0x00, 0x00, // WASM version
    };
    char error_buf[256];
    memset(error_buf, 0, sizeof(error_buf));

    wasm_module_t module = wasm_runtime_load(simple_wasm, sizeof(simple_wasm), error_buf, sizeof(error_buf));

    if (module != nullptr) {
        // Try to instantiate to trigger more error paths
        memset(error_buf, 0, sizeof(error_buf));
        wasm_module_inst_t inst = wasm_runtime_instantiate(module, 8192, 8192, error_buf, sizeof(error_buf));

        if (inst != nullptr) {
            wasm_runtime_deinstantiate(inst);
        }
        else {
            // Instantiation failed - verify error message was set
            ASSERT_GT(strlen(error_buf), static_cast<size_t>(0));
        }
        wasm_runtime_unload(module);
    }
    else {
        // Load failed - verify error message was set
        ASSERT_GT(strlen(error_buf), static_cast<size_t>(0));
    }
}

/*
 * COMPREHENSIVE COVERAGE TESTS FOR aot_lookup_function_with_idx
 * TARGET: Lines 1421-1452 in aot_runtime.c
 *
 * CALL PATHS EVALUATED:
 * 1. Direct call to aot_lookup_function_with_idx() [SELECTED - Direct testing]
 *    - Depth: 1 level
 *    - Complexity: LOW (minimal setup required)
 *    - Precision: HIGH (direct targeting of specific lines)
 *    - Rating: ⭐⭐⭐⭐
 *
 * 2. aot_get_function_instance() -> aot_lookup_function_with_idx() [Alternative]
 *    - Depth: 2 levels
 *    - Complexity: MEDIUM (requires valid AOT module setup)
 *    - Precision: MEDIUM (additional code paths involved)
 *    - Rating: ⭐⭐⭐
 *
 * SELECTED STRATEGY: Use aot_lookup_function_with_idx() directly with crafted AOTModuleInstance
 * REASON: Most precise targeting of lines 1421-1452 with minimal test complexity
 */

TEST_F(EnhancedAOTTest, LookupFunctionWithIdx_NoExportFunctions_ReturnsNull) {
    // Target: Line 1418-1419: if (module_inst->export_func_count == 0) return NULL;
    // This test ensures early return when no export functions exist

    // Create a minimal AOT module instance with no export functions
    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));

    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.export_func_count = 0; // No export functions

    // Call should return NULL immediately without entering lock section
    AOTFunctionInstance* result = aot_lookup_function_with_idx(&module_inst, 0);
    ASSERT_EQ(nullptr, result);
}

TEST_F(EnhancedAOTTest, LookupFunctionWithIdx_MapCreation_FindsFunction) {
    // Target: Lines 1442-1468: Map creation and population with successful search
    // This test triggers map allocation and creation, then finds the function

    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    AOTFunctionInstance export_funcs[2];

    // Setup module instance with export functions
    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));
    memset(export_funcs, 0, sizeof(export_funcs));

    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.export_func_count = 2;
    module_inst.export_functions = (WASMExportFuncInstance*)export_funcs;  // Proper cast

    // Setup export function data
    export_funcs[0].func_index = 100;
    export_funcs[1].func_index = 200;

    // Ensure export_func_maps is NULL to trigger map creation
    extra.export_func_maps = nullptr;

    // Test should create map and find function via binary search
    AOTFunctionInstance* result = aot_lookup_function_with_idx(&module_inst, 100);
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(100, result->func_index);
}

TEST_F(EnhancedAOTTest, LookupFunctionWithIdx_BinarySearchNotFound_ReturnsNull) {
    // Target: Lines 1466-1475: Binary search fails to find function
    // Tests the case where binary search via bsearch returns NULL

    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    AOTFunctionInstance export_funcs[2];

    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));
    memset(export_funcs, 0, sizeof(export_funcs));

    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.export_func_count = 2;
    module_inst.export_functions = (WASMExportFuncInstance*)export_funcs;

    export_funcs[0].func_index = 100;
    export_funcs[1].func_index = 200;

    extra.export_func_maps = nullptr; // Trigger map creation

    // Search for non-existent function index - binary search should fail
    AOTFunctionInstance* result = aot_lookup_function_with_idx(&module_inst, 999);
    ASSERT_EQ(nullptr, result);
}
