/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "wasm_export.h"
#include "aot_export.h"
#include "bh_read_file.h"
#include "aot_comp_option.h"

static std::string CWD;
static const char *LLVM_TEST_WASM = "/llvm_test.wasm";
static char *WASM_FILE;

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

class LLVMIntegrationOptimizationTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize WAMR runtime
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        if (!wasm_runtime_full_init(&init_args)) {
            FAIL() << "Failed to initialize WAMR runtime";
        }
        
        // Initialize AOT compiler
        if (!aot_compiler_init()) {
            FAIL() << "Failed to initialize AOT compiler";
        }
        
        // Initialize compilation options
        memset(&option, 0, sizeof(AOTCompOption));
        
        // Set default optimization values
        option.opt_level = 3;
        option.size_level = 3;
        option.output_format = AOT_FORMAT_FILE;
        option.bounds_checks = 2;
        option.enable_simd = true;
        option.enable_aux_stack_check = true;
        option.enable_bulk_memory = true;
        option.enable_thread_mgr = false;
        option.enable_tail_call = true;
        option.enable_ref_types = true;
        option.enable_llvm_pgo = false;
        option.disable_llvm_lto = false;
        
        // Initialize paths - Use existing WASM file that's already copied
        WASM_FILE = const_cast<char*>("main.wasm");
        
        // Initialize test data
        wasm_file_buf = nullptr;
        wasm_file_size = 0;
        wasm_module = nullptr;
        comp_data = nullptr;
        comp_ctx = nullptr;
        error_buf[0] = '\0';
    }
    
    void TearDown() override
    {
        // Clean up resources in reverse order
        if (comp_ctx != nullptr) {
            aot_destroy_comp_context(comp_ctx);
            comp_ctx = nullptr;
        }
        
        if (comp_data != nullptr) {
            aot_destroy_comp_data(comp_data);
            comp_data = nullptr;
        }
        
        if (wasm_module != nullptr) {
            wasm_runtime_unload(wasm_module);
            wasm_module = nullptr;
        }
        
        if (wasm_file_buf != nullptr) {
            BH_FREE(wasm_file_buf);
            wasm_file_buf = nullptr;
        }
        
        // Destroy AOT compiler
        aot_compiler_destroy();
        
        wasm_runtime_destroy();
    }
    
    bool load_wasm_file(const char *file_name)
    {
        // The WASM files are copied to the build directory, so use just the filename
        wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(file_name, &wasm_file_size);
        if (wasm_file_buf == nullptr || wasm_file_size == 0) {
            printf("Failed to read WASM file: %s\n", file_name);
            return false;
        }
        
        wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        if (wasm_module == nullptr) {
            printf("Failed to load WASM module: %s\n", error_buf);
        }
        return wasm_module != nullptr;
    }
    
    bool create_compilation_context(AOTCompOption *test_option)
    {
        if (wasm_module == nullptr) {
            printf("No WASM module loaded\n");
            return false;
        }
        
        comp_data = aot_create_comp_data(wasm_module, nullptr, false);
        if (comp_data == nullptr) {
            printf("Failed to create compilation data: %s\n", aot_get_last_error());
            return false;
        }
        
        comp_ctx = aot_create_comp_context(comp_data, test_option);
        if (comp_ctx == nullptr) {
            printf("Failed to create compilation context: %s\n", aot_get_last_error());
        }
        return comp_ctx != nullptr;
    }
    
    bool compile_wasm()
    {
        if (comp_ctx == nullptr) return false;
        return aot_compile_wasm(comp_ctx);
    }
    
    AOTCompOption option;
    unsigned char *wasm_file_buf;
    uint32_t wasm_file_size;
    wasm_module_t wasm_module;
    aot_comp_data_t comp_data;
    aot_comp_context_t comp_ctx;
    char error_buf[256];
};

// Test 14: LLVM Platform Specific Code Generation
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_platform_specific_code_generation)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test platform-specific code generation
    AOTCompOption platform_option = option;
    platform_option.target_arch = const_cast<char*>("x86_64");
    platform_option.opt_level = 2;
    
    ASSERT_TRUE(create_compilation_context(&platform_option));
    ASSERT_TRUE(compile_wasm()) << "Platform-specific code generation failed: " << aot_get_last_error();
    
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 15: LLVM Cross Compilation Support
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_cross_compilation_support)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test cross-compilation capabilities
    AOTCompOption cross_option = option;
    cross_option.target_arch = const_cast<char*>("x86_64");
    cross_option.target_abi = const_cast<char*>("gnu");
    
    ASSERT_TRUE(create_compilation_context(&cross_option));
    ASSERT_TRUE(compile_wasm()) << "Cross-compilation failed: " << aot_get_last_error();
    
    // Verify cross-compilation succeeded
    ASSERT_NE(comp_ctx, nullptr);
}
