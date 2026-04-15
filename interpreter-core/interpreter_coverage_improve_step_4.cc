/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_common.h"
#include "wasm_runtime_common.h"
#include "wasm_native.h"
#include "bh_read_file.h"
#include "wasm_runtime.h"
#include "bh_platform.h"
#include "wasm_export.h"
#include <unistd.h>
#include <fstream>

static std::string CWD;
static std::string WASM_FILE_DIR;

static int
app_argc;
static char **app_argv;

/*
 * Step 4: Module Loading and Validation Functions Coverage Tests
 * Target Functions:
 * 1. check_simd_shuffle_mask() - SIMD validation
 * 2. check_table_elem_type() - Table validation  
 * 3. check_table_index() - Table validation
 * 4. load_datacount_section() - Module loading
 * 5. load_table_segment_section() - Table initialization
 */

class ModuleLoadingValidationTest : public testing::TestWithParam<RunningMode>
{
protected:
    WAMRRuntimeRAII<512 * 1024> runtime;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint32_t stack_size = 8092, heap_size = 8092;
    char error_buf[128];
    std::string wasm_path;
    unsigned char *wasm_file_buf = nullptr;
    uint32_t wasm_file_size = 0;

    void SetUp() override
    {
        memset(error_buf, 0, sizeof(error_buf));
        char *current_dir = getcwd(nullptr, 0);
        CWD = std::string(current_dir);
        free(current_dir);
        WASM_FILE_DIR = CWD + "/";
    }

    void TearDown() override
    {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
            exec_env = nullptr;
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (wasm_file_buf != nullptr) {
            wasm_runtime_free(wasm_file_buf);
            wasm_file_buf = nullptr;
        }
    }

    bool load_wasm_file(const char *wasm_file)
    {
        wasm_path = WASM_FILE_DIR + wasm_file;
        
        // Read file using standard C++ file operations
        std::ifstream file(wasm_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }
        
        wasm_file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        wasm_file_buf = (unsigned char *)wasm_runtime_malloc(wasm_file_size);
        if (wasm_file_buf == nullptr) {
            return false;
        }
        
        if (!file.read((char *)wasm_file_buf, wasm_file_size)) {
            wasm_runtime_free(wasm_file_buf);
            wasm_file_buf = nullptr;
            return false;
        }

        module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        return module != nullptr;
    }

    bool init_exec_env()
    {
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
        if (module_inst == nullptr) {
            return false;
        }

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        return exec_env != nullptr;
    }
};

// Test 4: Datacount Section Loading
TEST_P(ModuleLoadingValidationTest, LoadDatacountSection_ValidSection_LoadsSuccessfully)
{
    // Load datacount test module
    ASSERT_TRUE(load_wasm_file("datacount_test.wasm"));
    ASSERT_TRUE(init_exec_env());

    // Set running mode
    RunningMode mode = GetParam();
    wasm_runtime_set_running_mode(module_inst, mode);

    // Look up and call datacount validation function
    wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_datacount_section");
    ASSERT_TRUE(func != nullptr);

    uint32_t wasm_argv[1];
    wasm_argv[0] = 2;  // Expected data segment count

    ASSERT_TRUE(wasm_runtime_call_wasm(exec_env, func, 1, wasm_argv));
    
    // Verify result - should return 1 for successful load
    uint32_t result = wasm_argv[0];
    ASSERT_EQ(1, result);
}

INSTANTIATE_TEST_SUITE_P(RunningModeTest, ModuleLoadingValidationTest,
                         testing::Values(Mode_Interp, Mode_Fast_JIT, Mode_LLVM_JIT, Mode_Multi_Tier_JIT));

int
main(int argc, char **argv)
{
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        CWD = std::string(buffer);
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
