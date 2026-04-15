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

// Enhanced test fixture for aot_compiler.c functions
class EnhancedAotCompilerTest : public testing::Test {
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

public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

// ============================================================================
// TEST CASES FOR aot_emit_object_file COVERAGE
// ============================================================================

/******
 * Test Case: aot_emit_object_file_DefaultLLVMEmission_Success
 * Source: core/iwasm/compilation/aot_compiler.c
 * Functional Purpose: Validates that aot_emit_object_file() correctly handles the default
 *                     LLVM target machine emission path with stack usage file option set.
 *                     Sets WAMRC_LLC_COMPILER env var to exercise comp context creation
 *                     with LLC awareness, but the actual emit falls through to default path.
 * Call Path: aot_emit_object_file() -> LLVMTargetMachineEmitToFile()
 * Coverage Goal: Exercise the default LLVM emission success path
 ******/
TEST_F(EnhancedAotCompilerTest, aot_emit_object_file_DefaultLLVMEmission_Success) {
    wasm_module_t module = createTestModule();
    ASSERT_NE(module, nullptr);

    aot_comp_data_t comp_data = aot_create_comp_data(module, nullptr, false);
    ASSERT_NE(comp_data, nullptr);

    AOTCompOption option = {};
    option.opt_level = 3;
    option.size_level = 3;
    option.output_format = AOT_FORMAT_FILE;
    option.bounds_checks = 2;
    option.enable_simd = false;
    option.enable_aux_stack_check = true;
    option.enable_bulk_memory = false;
    option.enable_ref_types = false;
    option.enable_gc = false;

    // Set stack usage file option for additional coverage in comp context
    char stack_usage_file[] = "/tmp/test_stack_usage.su";
    option.stack_usage_file = stack_usage_file;

    // Set LLC env var for comp context creation coverage
    setenv("WAMRC_LLC_COMPILER", "/usr/bin/llc", 1);

    aot_comp_context_t comp_ctx = aot_create_comp_context(comp_data, &option);
    ASSERT_NE(comp_ctx, nullptr);

    bool compile_result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(compile_result);

    char obj_file_name[] = "/tmp/wamr_default_emit_test.o";

    bool emit_result = aot_emit_object_file(comp_ctx, obj_file_name);
    EXPECT_TRUE(emit_result);

    unlink(obj_file_name);
    unsetenv("WAMRC_LLC_COMPILER");

    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    wasm_runtime_unload(module);
}

/******
 * Test Case: aot_emit_object_file_ExternalASMCompiler_Success
 * Source: core/iwasm/compilation/aot_compiler.c:4265-4303
 * Target Lines: 4265 (external_asm_compiler check), 4268-4270 (tempfile generation),
 *               4273-4283 (LLVM assembly emission), 4285-4290 (command construction),
 *               4292-4300 (external ASM compilation and cleanup), 4303 (return true)
 * Functional Purpose: Validates that aot_emit_object_file() correctly handles
 *                     external ASM compiler compilation path by setting up the
 *                     environment for external ASM compilation and exercising
 *                     the complete ASM emission and compilation workflow.
 * Call Path: aot_emit_object_file() -> LLVMTargetMachineEmitToFile() -> bh_system()
 * Coverage Goal: Exercise external ASM compiler path completely
 ******/
TEST_F(EnhancedAotCompilerTest, aot_emit_object_file_ExternalASMCompiler_Success) {
    wasm_module_t module = createTestModule();
    ASSERT_NE(module, nullptr);

    aot_comp_data_t comp_data = aot_create_comp_data(module, nullptr, false);
    ASSERT_NE(comp_data, nullptr);

    AOTCompOption option = {};
    option.opt_level = 3;
    option.size_level = 3;
    option.output_format = AOT_FORMAT_FILE;
    option.bounds_checks = 2;
    option.enable_simd = false;
    option.enable_aux_stack_check = true;
    option.enable_bulk_memory = false;
    option.enable_ref_types = false;
    option.enable_gc = false;

    // Ensure no LLC compiler is set to avoid LLC path
    unsetenv("WAMRC_LLC_COMPILER");

    // Set up external ASM compiler environment
    setenv("WAMRC_ASM_COMPILER", "/usr/bin/gcc", 1);

    aot_comp_context_t comp_ctx = aot_create_comp_context(comp_data, &option);
    ASSERT_NE(comp_ctx, nullptr);

    bool compile_result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(compile_result);

    char obj_file_name[] = "/tmp/wamr_asm_compiler_test.o";

    bool emit_result = aot_emit_object_file(comp_ctx, obj_file_name);
    EXPECT_TRUE(emit_result);

    unlink(obj_file_name);
    unsetenv("WAMRC_ASM_COMPILER");

    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    wasm_runtime_unload(module);
}
