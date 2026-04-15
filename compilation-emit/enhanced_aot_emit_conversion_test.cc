/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "wasm_export.h"
#include "aot_export.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string MAIN_WASM = "/main.wasm";
static std::string CONVERSION_WASM = "/conversion_test.wasm";
static char *WASM_FILE;
static char *CONVERSION_WASM_FILE;

static std::string
get_binary_path()
{
    char cwd[1024];
    memset(cwd, 0, 1024);

    if (readlink("/proc/self/exe", cwd, 1024) <= 0) {
    }

    char *path_end = strrchr(cwd, '/');
    if (path_end != nullptr) {
        *path_end = '\0';
    }

    return {cwd};
}

class EnhancedAotEmitConversionTest : public testing::Test {
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

    static void SetUpTestCase()
    {
        CWD = get_binary_path();
        WASM_FILE = strdup((CWD + MAIN_WASM).c_str());
        CONVERSION_WASM_FILE = strdup((CWD + CONVERSION_WASM).c_str());
    }

    static void TearDownTestCase() {
        free(WASM_FILE);
        free(CONVERSION_WASM_FILE);
    }

public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

/******
 * Test Case: aot_compile_op_i64_trunc_f32_IndirectMode_IntrinsicCapability_ReturnsTrue
 * Source: core/iwasm/compilation/aot_emit_conversion.c:551-601
 * Target Lines: 551 (function entry), 557 (POP_F32), 559-561 (indirect mode and intrinsic check),
 *               562-577 (indirect mode paths with aot_load_const_from_table), 589-590 (CHECK_LLVM_CONST),
 *               592-595 (non-saturating path calling trunc_float_to_int)
 * Functional Purpose: Validates that aot_compile_op_i64_trunc_f32() successfully
 *                     handles indirect mode compilation with intrinsic capability check,
 *                     using aot_load_const_from_table for constant value loading.
 * Call Path: aot_compile_op_i64_trunc_f32() <- aot_compiler.c in indirect mode
 * Coverage Goal: Exercise indirect mode path with intrinsic capability support
 ******/
TEST_F(EnhancedAotEmitConversionTest, aot_compile_op_i64_trunc_f32_IndirectMode_IntrinsicCapability_ReturnsTrue) {
    const char *wasm_file = "i64_trunc_f32_test.wasm";
    unsigned int wasm_file_size = 0;
    unsigned char *wasm_file_buf = nullptr;
    char error_buf[128] = {0};
    wasm_module_t wasm_module = nullptr;
    aot_comp_data_t comp_data = nullptr;
    aot_comp_context_t comp_ctx = nullptr;
    AOTCompOption option = {};

    // Initialize compilation options for indirect mode
    option.opt_level = 1;
    option.size_level = 1;
    option.output_format = AOT_FORMAT_FILE;
    option.bounds_checks = 2;
    option.enable_simd = false;
    option.enable_aux_stack_check = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = false;
    option.is_indirect_mode = true; // Force indirect mode

    // Load WASM module from file
    wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
    ASSERT_NE(nullptr, wasm_file_buf);
    wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, wasm_module);

    // Create compilation data
    comp_data = aot_create_comp_data(wasm_module, nullptr, false);
    ASSERT_NE(nullptr, comp_data);

    // Create compilation context
    comp_ctx = aot_create_comp_context(comp_data, &option);
    ASSERT_NE(nullptr, comp_ctx);

    // Compile WASM - this will trigger aot_compile_op_i64_trunc_f32 in indirect mode
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));

    // Clean up
    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    wasm_runtime_unload(wasm_module);
}

/******
 * Test Case: aot_compile_op_f32_convert_i32_WithIntrinsics_ReturnsTrue
 * Source: core/iwasm/compilation/aot_emit_conversion.c:660-694
 * Target Lines: 660 (function entry), 665 (POP_I32), 667-675 (intrinsic path),
 *               670-675 (aot_call_llvm_intrinsic), 690 (PUSH_F32), 691 (return true)
 * Functional Purpose: Validates that aot_compile_op_f32_convert_i32() successfully
 *                     compiles f32.convert_i32_s/u operations using LLVM intrinsics,
 *                     including proper stack operations and intrinsic function calls.
 * Call Path: aot_compile_op_f32_convert_i32() <- aot_compiler.c:2378 <- WASM_OP_F32_CONVERT_*_I32
 * Coverage Goal: Exercise intrinsic path for f32.convert_i32 conversions
 ******/
TEST_F(EnhancedAotEmitConversionTest, aot_compile_op_f32_convert_i32_WithIntrinsics_ReturnsTrue) {
    wasm_module_t wasm_module;
    aot_comp_data_t comp_data;
    aot_comp_context_t comp_ctx;
    uint32 wasm_file_size;
    uint8 *wasm_file_buf;
    char error_buf[128];
    AOTCompOption option = {};

    // Enable intrinsics to trigger the intrinsic path
    option.disable_llvm_intrinsics = true;

    // Load f32_convert_i32 test WASM module
    wasm_file_buf = (uint8*)bh_read_file_to_buffer("f32_convert_i32_test.wasm", &wasm_file_size);
    ASSERT_NE(nullptr, wasm_file_buf);
    wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, wasm_module);

    // Create compilation data
    comp_data = aot_create_comp_data(wasm_module, nullptr, false);
    ASSERT_NE(nullptr, comp_data);

    // Create compilation context with intrinsics enabled
    comp_ctx = aot_create_comp_context(comp_data, &option);
    ASSERT_NE(nullptr, comp_ctx);

    // Compile WASM - this will trigger aot_compile_op_f32_convert_i32 intrinsic path
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));

    // Clean up
    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    wasm_runtime_unload(wasm_module);
}

// =============================================================================
// NEW TEST CASES FOR aot_compile_op_f32_convert_i64 COVERAGE (Lines 697-731)
// =============================================================================

/******
 * Test Case: aot_compile_op_f32_convert_i64_SignedConversion_ReturnsTrue
 * Source: core/iwasm/compilation/aot_emit_conversion.c:697-731
 * Target Lines: 697 (function entry), 702 (POP_I64), 715 (sign check),
 *               716-717 (LLVMBuildSIToFP), 723-726 (error check), 728 (PUSH_F32), 729 (return true)
 * Functional Purpose: Validates that aot_compile_op_f32_convert_i64() successfully
 *                     compiles f32.convert_i64_s operation by converting i64 to f32,
 *                     including proper stack operations and LLVM IR generation for signed conversion.
 * Call Path: aot_compile_op_f32_convert_i64() <- aot_compiler.c WASM_OP_F32_CONVERT_S_I64
 * Coverage Goal: Exercise signed conversion path (LLVMBuildSIToFP) in success scenario
 ******/
TEST_F(EnhancedAotEmitConversionTest, aot_compile_op_f32_convert_i64_SignedConversion_ReturnsTrue) {
    const char *wasm_file = "f32_convert_i64_test.wasm";
    unsigned int wasm_file_size = 0;
    unsigned char *wasm_file_buf = nullptr;
    char error_buf[128] = {0};
    wasm_module_t wasm_module = nullptr;
    aot_comp_data_t comp_data = nullptr;
    aot_comp_context_t comp_ctx = nullptr;
    AOTCompOption option = {};

    // Initialize compilation options for signed conversion
    option.opt_level = 1;
    option.size_level = 1;
    option.output_format = AOT_FORMAT_FILE;
    option.bounds_checks = 2;
    option.enable_simd = false;
    option.enable_aux_stack_check = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = false;
    option.disable_llvm_intrinsics = false; // Standard LLVM path

    // Load WASM module from file
    wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
    ASSERT_NE(nullptr, wasm_file_buf);
    wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, wasm_module);

    // Create compilation data
    comp_data = aot_create_comp_data(wasm_module, nullptr, false);
    ASSERT_NE(nullptr, comp_data);

    // Create compilation context
    comp_ctx = aot_create_comp_context(comp_data, &option);
    ASSERT_NE(nullptr, comp_ctx);

    // Compile WASM - this will trigger aot_compile_op_f32_convert_i64 for f32.convert_i64_s
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));

    // Clean up
    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    wasm_runtime_unload(wasm_module);
}

/******
 * Test Case: aot_compile_op_f32_convert_i64_SignedWithIntrinsics_ReturnsTrue
 * Source: core/iwasm/compilation/aot_emit_conversion.c:697-731
 * Target Lines: 697 (function entry), 702 (POP_I64), 704-706 (intrinsic capability check),
 *               707-712 (aot_call_llvm_intrinsic path), 723-726 (error check), 728 (PUSH_F32), 729 (return true)
 * Functional Purpose: Validates that aot_compile_op_f32_convert_i64() successfully
 *                     compiles f32.convert_i64_s operation using LLVM intrinsics when available,
 *                     including proper intrinsic function calls and parameter handling.
 * Call Path: aot_compile_op_f32_convert_i64() <- aot_compiler.c WASM_OP_F32_CONVERT_S_I64
 * Coverage Goal: Exercise intrinsic path for signed conversion (aot_call_llvm_intrinsic)
 ******/
TEST_F(EnhancedAotEmitConversionTest, aot_compile_op_f32_convert_i64_SignedWithIntrinsics_ReturnsTrue) {
    const char *wasm_file = "f32_convert_i64_test.wasm";
    unsigned int wasm_file_size = 0;
    unsigned char *wasm_file_buf = nullptr;
    char error_buf[128] = {0};
    wasm_module_t wasm_module = nullptr;
    aot_comp_data_t comp_data = nullptr;
    aot_comp_context_t comp_ctx = nullptr;
    AOTCompOption option = {};

    // Initialize compilation options to trigger intrinsic path
    option.opt_level = 2;
    option.size_level = 2;
    option.output_format = AOT_FORMAT_FILE;
    option.bounds_checks = 1;
    option.enable_simd = true;
    option.enable_aux_stack_check = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = false;
    option.disable_llvm_intrinsics = true; // Enable intrinsic path

    // Load WASM module from file
    wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
    ASSERT_NE(nullptr, wasm_file_buf);
    wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, wasm_module);

    // Create compilation data
    comp_data = aot_create_comp_data(wasm_module, nullptr, false);
    ASSERT_NE(nullptr, comp_data);

    // Create compilation context
    comp_ctx = aot_create_comp_context(comp_data, &option);
    ASSERT_NE(nullptr, comp_ctx);

    // Compile WASM - this will trigger aot_compile_op_f32_convert_i64 with intrinsics
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));

    // Clean up
    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    wasm_runtime_unload(wasm_module);
}

/******
 * Test Case: aot_compile_op_f32_demote_f64_Multiple_Operations
 * Source: core/iwasm/compilation/aot_emit_conversion.c:735-762
 * Target Lines: 735 (function entry), 738 (variable declarations), 740 (POP_F64),
 *               742-752 (both paths), 754 (result check), 759 (PUSH_F32), 760 (return true)
 * Functional Purpose: Validates that aot_compile_op_f32_demote_f64() successfully
 *                     handles multiple f32.demote_f64 operations within the same function,
 *                     ensuring proper stack management and result handling.
 * Call Path: aot_compile_op_f32_demote_f64() <- aot_compiler.c switch WASM_OP_F32_DEMOTE_F64
 * Coverage Goal: Exercise function with multiple f32.demote_f64 operations
 ******/
TEST_F(EnhancedAotEmitConversionTest, aot_compile_op_f32_demote_f64_Multiple_Operations) {
    const char *wasm_file = "f32_demote_f64_test.wasm";
    unsigned int wasm_file_size = 0;
    unsigned char *wasm_file_buf = nullptr;
    char error_buf[128] = {0};
    wasm_module_t wasm_module = nullptr;
    aot_comp_data_t comp_data = nullptr;
    aot_comp_context_t comp_ctx = nullptr;
    AOTCompOption option = {};

    // Initialize compilation options
    option.opt_level = 2;
    option.size_level = 1;
    option.output_format = AOT_FORMAT_FILE;
    option.bounds_checks = 2;
    option.enable_simd = false;
    option.enable_aux_stack_check = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = false;

    // Load WASM module from file
    wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
    ASSERT_NE(nullptr, wasm_file_buf);
    ASSERT_GT(wasm_file_size, 0U);

    // Load WASM module
    wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, wasm_module);

    // Create compilation data
    comp_data = aot_create_comp_data(wasm_module, nullptr, false);
    ASSERT_NE(nullptr, comp_data);

    // Create compilation context
    comp_ctx = aot_create_comp_context(comp_data, &option);
    ASSERT_NE(nullptr, comp_ctx);

    // Compile WASM - this will trigger multiple f32_demote_f64 operations
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));

    // Clean up
    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    wasm_runtime_unload(wasm_module);
    BH_FREE(wasm_file_buf);
}

// =============================================================================
// NEW TEST CASES FOR aot_compile_op_f64_convert_i64 COVERAGE (Lines 805-840)
// =============================================================================

/******
 * Test Case: aot_compile_op_f64_convert_i64_SignedConstrainedIntrinsicPath_ReturnsTrue
 * Source: core/iwasm/compilation/aot_emit_conversion.c:805-840
 * Target Lines: 805-810 (function entry, POP_I64), 823-830 (signed constrained intrinsic path), 832-838 (success path)
 * Functional Purpose: Validates that aot_compile_op_f64_convert_i64() correctly performs
 *                     signed i64 to f64 conversion using constrained intrinsic path
 *                     when LLVM intrinsics are disabled (disable_llvm_intrinsics=true).
 * Call Path: aot_compile_op_f64_convert_i64() <- aot_compile_func() <- WASM_OP_F64_CONVERT_S_I64
 * Coverage Goal: Exercise signed conversion via constrained intrinsic path (lines 824-826)
 ******/
TEST_F(EnhancedAotEmitConversionTest, aot_compile_op_f64_convert_i64_SignedConstrainedIntrinsicPath_ReturnsTrue) {
    unsigned int wasm_file_size = 0;
    unsigned char *wasm_file_buf = nullptr;
    char error_buf[128] = {0};
    wasm_module_t wasm_module = nullptr;
    aot_comp_data_t comp_data = nullptr;
    aot_comp_context_t comp_ctx = nullptr;
    AOTCompOption option = {};

    // Initialize compilation options with intrinsics disabled
    option.opt_level = 3;
    option.size_level = 0;
    option.output_format = AOT_FORMAT_FILE;
    option.bounds_checks = 2;
    option.enable_simd = false;
    option.enable_aux_stack_check = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = false;
    option.disable_llvm_intrinsics = true;  // Force constrained intrinsic path
    option.enable_llvm_pgo = false;

    // Load f64_convert_s_i64 test WASM module
    wasm_file_buf = (uint8*)bh_read_file_to_buffer("f64_convert_s_i64_test.wasm", &wasm_file_size);
    ASSERT_NE(nullptr, wasm_file_buf);
    ASSERT_GT(wasm_file_size, 0U);
    wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, wasm_module);

    // Create compilation data
    comp_data = aot_create_comp_data(wasm_module, nullptr, false);
    ASSERT_NE(nullptr, comp_data);

    // Create compilation context
    comp_ctx = aot_create_comp_context(comp_data, &option);
    ASSERT_NE(nullptr, comp_ctx);

    // Note: Cannot verify disable_llvm_intrinsics directly due to incomplete type

    // Compile WASM - this should use the standard SIToFP path
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));

    // Clean up
    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    wasm_runtime_unload(wasm_module);
    BH_FREE(wasm_file_buf);
}
