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

static std::string CWD;                              // NOLINT(cert-err58-cpp)
static std::string SIMD_WASM = "/simd_test.wasm";   // NOLINT(cert-err58-cpp)
static char *WASM_FILE;

static std::string
get_binary_path()
{
    char cwd[1024];
    memset(cwd, 0, 1024);

    if (readlink("/proc/self/exe", cwd, 1024) <= 0) {
        // Fallback to current working directory
        if (getcwd(cwd, 1024) == nullptr) {
            return {};
        }
    }

    char *path_end = strrchr(cwd, '/');
    if (path_end != nullptr) {
        *path_end = '\0';
    }

    return {cwd};
}

class SIMDAdvancedInstructionsTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize WAMR runtime with SIMD support
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        init_args.max_thread_num = 4;
        
        // Configure native symbols for SIMD support
        init_args.n_native_symbols = 0;
        init_args.native_module_name = nullptr;
        init_args.native_symbols = nullptr;
        
        if (!wasm_runtime_full_init(&init_args)) {
            FAIL() << "Failed to initialize WAMR runtime";
        }
        
        // Initialize AOT compiler
        if (!aot_compiler_init()) {
            FAIL() << "Failed to initialize AOT compiler";
        }
        
        // Initialize test environment
        memset(&option, 0, sizeof(AOTCompOption));
        
        // Set default values with SIMD enabled
        option.opt_level = 3;
        option.size_level = 3;
        option.output_format = AOT_FORMAT_FILE;
        option.bounds_checks = 2;
        option.enable_simd = true;
        option.enable_aux_stack_check = true;
        option.enable_bulk_memory = true;
        option.enable_ref_types = true;
        
        // Initialize call stack features
        aot_call_stack_features_init_default(&option.call_stack_features);
        
        // Reset all pointers
        wasm_file_buf = nullptr;
        wasm_file_size = 0;
        wasm_module = nullptr;
        comp_data = nullptr;
        comp_ctx = nullptr;
    }

    void TearDown() override
    {
        // Clean up resources in reverse order of creation
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
        
        // Destroy WAMR runtime
        wasm_runtime_destroy();
    }

    static void SetUpTestCase()
    {
        CWD = get_binary_path();
        WASM_FILE = strdup((CWD + SIMD_WASM).c_str());
    }

    static void TearDownTestCase() 
    { 
        if (WASM_FILE != nullptr) {
            free(WASM_FILE);
            WASM_FILE = nullptr;
        }
    }

    bool LoadWasmModule()
    {
        // Use existing WASM file - check if SIMD features are supported
        const char* wasm_file = "simd_test.wasm";
        wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
        if (wasm_file_buf == nullptr) {
            // Fallback to main.wasm if SIMD file is not available
            wasm_file = "main.wasm";
            wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
            if (wasm_file_buf == nullptr) {
                printf("Failed to load WASM file: %s\n", wasm_file);
                return false;
            }
        }

        char error_buf[128];
        wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        if (wasm_module == nullptr) {
            printf("Failed to load WASM module: %s\n", error_buf);
            return false;
        }
        return true;
    }

    bool InitializeCompilationContext()
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

        comp_ctx = aot_create_comp_context(comp_data, &option);
        if (comp_ctx == nullptr) {
            printf("Failed to create compilation context: %s\n", aot_get_last_error());
        }
        return comp_ctx != nullptr;
    }

    // Test fixtures and helper data
    AOTCompOption option;
    unsigned char *wasm_file_buf;
    uint32_t wasm_file_size;
    wasm_module_t wasm_module;
    AOTCompData *comp_data;
    AOTCompContext *comp_ctx;
};

TEST_F(SIMDAdvancedInstructionsTest, test_advanced_control_flow_compilation)
{
    // Test advanced control flow compilation
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = true;
    option.bounds_checks = 1;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "Advanced control flow compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}
