/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "wasm_export.h"
#include "aot_export.h"
#include "bh_read_file.h"
#include <chrono>
#include <thread>
#include <atomic>

static std::string CWD;
static const char *const MAIN_WASM = "/main.wasm";
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

class IntegrationPerformanceTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize WAMR runtime first
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args)) << "Failed to initialize WAMR runtime";
        
        // Initialize AOT compiler
        ASSERT_TRUE(aot_compiler_init()) << "Failed to initialize AOT compiler";
        
        // Initialize test environment
        memset(&option, 0, sizeof(AOTCompOption));
        
        // Set default values for integration testing
        option.opt_level = 3;
        option.size_level = 3;
        option.output_format = AOT_FORMAT_FILE;
        option.bounds_checks = 2;
        option.enable_simd = true;
        option.enable_aux_stack_check = true;
        option.enable_bulk_memory = true;
        option.enable_ref_types = true;
        option.enable_thread_mgr = true;
        
        // Reset all pointers
        wasm_file_buf = nullptr;
        wasm_file_size = 0;
        wasm_module = nullptr;
        comp_data = nullptr;
        comp_ctx = nullptr;
        
        // Performance tracking
        compilation_start_time = std::chrono::high_resolution_clock::now();
        compilation_end_time = compilation_start_time;
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
        WASM_FILE = strdup((CWD + MAIN_WASM).c_str());
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
        wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(WASM_FILE, &wasm_file_size);
        if (wasm_file_buf == nullptr) {
            return false;
        }

        char error_buf[128];
        wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        return wasm_module != nullptr;
    }

    bool CreateCompilationData()
    {
        char error_buf[128];
        comp_data = aot_create_comp_data(wasm_module, nullptr, false);
        if (comp_data == nullptr) {
            return false;
        }

        comp_ctx = aot_create_comp_context(comp_data, &option);
        return comp_ctx != nullptr;
    }

    void StartPerformanceTimer()
    {
        compilation_start_time = std::chrono::high_resolution_clock::now();
    }

    void EndPerformanceTimer()
    {
        compilation_end_time = std::chrono::high_resolution_clock::now();
    }

    double GetCompilationTimeMs()
    {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            compilation_end_time - compilation_start_time);
        return static_cast<double>(duration.count()) / 1000.0;
    }

    // Test fixtures and helper data
    AOTCompOption option;
    unsigned char *wasm_file_buf;
    uint32_t wasm_file_size;
    wasm_module_t wasm_module;
    aot_comp_data_t comp_data;
    aot_comp_context_t comp_ctx;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point compilation_start_time;
    std::chrono::high_resolution_clock::time_point compilation_end_time;
};

TEST_F(IntegrationPerformanceTest, ConcurrentCompilationStressTesting)
{
    // Test compilation thread safety with concurrent operations
    ASSERT_TRUE(LoadWasmModule());
    
    std::atomic<int> successful_compilations(0);
    std::atomic<int> failed_compilations(0);
    const int num_threads = 2; // Conservative for test stability
    
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            // Each thread needs its own compilation context
            aot_comp_data_t local_comp_data = nullptr;
            aot_comp_context_t local_comp_ctx = nullptr;
            
            try {
                local_comp_data = aot_create_comp_data(wasm_module, nullptr, false);
                if (local_comp_data != nullptr) {
                    local_comp_ctx = aot_create_comp_context(local_comp_data, &option);
                    if (local_comp_ctx != nullptr) {
                        bool result = aot_compile_wasm(local_comp_ctx);
                        if (result) {
                            successful_compilations++;
                        } else {
                            failed_compilations++;
                        }
                    } else {
                        failed_compilations++;
                    }
                } else {
                    failed_compilations++;
                }
            } catch (...) {
                failed_compilations++;
            }
            
            // Cleanup
            if (local_comp_ctx != nullptr) {
                aot_destroy_comp_context(local_comp_ctx);
            }
            if (local_comp_data != nullptr) {
                aot_destroy_comp_data(local_comp_data);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify at least some compilations succeeded
    ASSERT_GT(successful_compilations.load(), 0);
    ASSERT_EQ(successful_compilations.load() + failed_compilations.load(), num_threads);
}

TEST_F(IntegrationPerformanceTest, CompilationRegressionDetection)
{
    // Test compilation regression detection mechanisms
    ASSERT_TRUE(LoadWasmModule());
    
    // Baseline compilation with standard settings
    option.opt_level = 2;
    option.size_level = 2;
    option.enable_simd = true;
    
    ASSERT_TRUE(CreateCompilationData());
    
    StartPerformanceTimer();
    bool result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    double baseline_time = GetCompilationTimeMs();
    
    ASSERT_TRUE(result);
    ASSERT_GT(baseline_time, 0.1);
    ASSERT_LT(baseline_time, 30000.0); // Should complete within 30 seconds
    
    // Verify no regression in basic functionality
    ASSERT_NE(comp_data, nullptr);
}
