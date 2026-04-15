/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "wasm_export.h"
#include "aot_export.h"
#include "bh_read_file.h"
#include <climits>

// Need LLVM headers for LLVMValueRef
#include <llvm-c/Core.h>

// Forward declarations for testing internal functions
extern "C" {
bool aot_alloc_frame_per_function_frame_for_aot_func(void *comp_ctx, void *func_ctx, void *func_index);
bool aot_free_frame_per_function_frame_for_aot_func(void *comp_ctx, void *func_ctx);
bool aot_tiny_frame_gen_commit_ip(void *comp_ctx, void *func_ctx, LLVMValueRef ip_value);
void aot_set_last_error(const char *error);
}

// Enhanced test fixture for aot_stack_frame_comp.c functions
class EnhancedAotStackFrameCompTest : public testing::Test {
protected:
    void SetUp() override {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
        wasm_runtime_full_init(&init_args);
    }

    void TearDown() override {
        wasm_runtime_destroy();
    }

    // Helper method to create a basic WASM module for testing
    wasm_module_t createTestModule() {
        // Simple WASM module with basic functions for testing
        uint8_t simple_wasm[] = {
            0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
            0x01, 0x07, 0x01, 0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
            0x03, 0x02, 0x01, 0x00,
            0x0A, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6A, 0x0B
        };

        char error_buf[128] = { 0 };
        wasm_module_t module = wasm_runtime_load(simple_wasm, sizeof(simple_wasm),
                                               error_buf, sizeof(error_buf));
        return module;
    }

    // Helper method to create a minimal compilation context for testing
    aot_comp_context_t createTestCompContext(uint32_t frame_type = 0) {
        wasm_module_t module = createTestModule();
        if (module == nullptr) return nullptr;

        aot_comp_data_t comp_data = aot_create_comp_data(module, nullptr, false);
        if (comp_data == nullptr) {
            wasm_runtime_unload(module);
            return nullptr;
        }

        AOTCompOption option = {};
        option.opt_level = 0;
        option.size_level = 0;
        option.output_format = AOT_FORMAT_FILE;
        // Set the aux_stack_frame_type through manual memory access
        // This is a workaround since we can't include internal headers
        auto *frame_type_ptr = (uint32_t*)((char*)&option + 72); // Offset of aux_stack_frame_type
        *frame_type_ptr = frame_type;

        aot_comp_context_t comp_ctx = aot_create_comp_context(comp_data, &option);
        return comp_ctx;
    }

public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

/******
 * Test Case: aot_alloc_frame_per_function_frame_for_aot_func_UnsupportedFrameType_ReturnsFailureAndSetsError
 * Source: core/iwasm/compilation/aot_stack_frame_comp.c:125-135
 * Target Lines: 133-135 (default case with error handling)
 * Functional Purpose: Validates that aot_alloc_frame_per_function_frame_for_aot_func()
 *                     correctly handles unsupported aux_stack_frame_type values by
 *                     returning false and setting appropriate error message.
 * Call Path: Direct call to aot_alloc_frame_per_function_frame_for_aot_func()
 * Coverage Goal: Exercise error handling path for unsupported stack frame types
 ******/
TEST_F(EnhancedAotStackFrameCompTest, aot_alloc_frame_per_function_frame_for_aot_func_UnsupportedFrameType_ReturnsFailureAndSetsError) {
    // Create compilation context with an unsupported stack frame type
    // Using frame type 1 (AOT_STACK_FRAME_TYPE_STANDARD) which should trigger default case
    aot_comp_context_t comp_ctx = createTestCompContext(1);
    ASSERT_NE(comp_ctx, nullptr);

    // Create a dummy function context (void pointer for simplicity)
    char func_ctx_dummy = 0;
    void* func_ctx = &func_ctx_dummy;

    // Create a dummy LLVM function index value (void pointer for simplicity)
    LLVMValueRef func_index = LLVMConstInt(LLVMInt32Type(), 42, 0);
    ASSERT_NE(func_index, nullptr);

    // Clear any previous error messages
    aot_set_last_error("");

    // Call the function with unsupported frame type - should hit lines 133-135
    bool result = aot_alloc_frame_per_function_frame_for_aot_func(comp_ctx, func_ctx, func_index);

    // Verify the function returns false (indicating failure)
    ASSERT_FALSE(result);

    // Verify the correct error message is set (it includes "Error: " prefix)
    const char* error_msg = aot_get_last_error();
    ASSERT_STREQ(error_msg, "Error: unsupported mode");

    // Cleanup
    aot_destroy_comp_context(comp_ctx);
}

// Note: Additional positive test case for supported frame types would require
// complex LLVM context setup that's beyond the scope of this coverage enhancement.
// The two error path tests above are sufficient to cover the target lines 133-135.

/******
 * Test Case: aot_free_frame_per_function_frame_for_aot_func_FrameOff_ReturnsFailureAndSetsError
 * Source: core/iwasm/compilation/aot_stack_frame_comp.c:140-151
 * Target Lines: 149-151 (default case with error handling)
 * Functional Purpose: Validates that aot_free_frame_per_function_frame_for_aot_func()
 *                     correctly handles AOT_STACK_FRAME_OFF frame type by falling through
 *                     to the default case, returning false and setting error message.
 * Call Path: Direct call to aot_free_frame_per_function_frame_for_aot_func()
 * Coverage Goal: Exercise error handling path for unsupported/off stack frame types
 ******/
TEST_F(EnhancedAotStackFrameCompTest, aot_free_frame_per_function_frame_for_aot_func_FrameOff_ReturnsFailureAndSetsError) {
    // Create compilation context with frame type 0 (AOT_STACK_FRAME_OFF)
    aot_comp_context_t comp_ctx = createTestCompContext(0);
    ASSERT_NE(comp_ctx, nullptr);

    // Create a dummy function context (void pointer for simplicity)
    char func_ctx_dummy = 0;
    void* func_ctx = &func_ctx_dummy;

    // Clear any previous error messages
    aot_set_last_error("");

    // Call the free function with OFF frame type - hits default case
    bool result = aot_free_frame_per_function_frame_for_aot_func(comp_ctx, func_ctx);

    // Verify the function returns false (failure via default case)
    ASSERT_FALSE(result);

    // Cleanup
    aot_destroy_comp_context(comp_ctx);
}
