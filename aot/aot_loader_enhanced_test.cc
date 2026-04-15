/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "wasm_export.h"
#include "bh_read_file.h"

#if WASM_ENABLE_AOT != 0
#include "aot_runtime.h"
#endif

class AOTLoaderEnhancedTest : public testing::Test
{
protected:
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
        
        wasm_file_buf = nullptr;
        wasm_file_size = 0;
        aot_file_buf = nullptr;
        aot_file_size = 0;
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        error_buf[0] = '\0';
    }
    
    void TearDown() override
    {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (wasm_file_buf != nullptr) {
            wasm_runtime_free(wasm_file_buf);
        }
        if (aot_file_buf != nullptr) {
            wasm_runtime_free(aot_file_buf);
        }
        
        wasm_runtime_destroy();
    }
    
    void loadAOTFile(const char* filename)
    {
        aot_file_buf = (uint8*)bh_read_file_to_buffer(filename, &aot_file_size);
        if (aot_file_buf == nullptr) {
            return;
        }
        
        module = wasm_runtime_load(aot_file_buf, aot_file_size, error_buf, sizeof(error_buf));
    }
    
    void instantiateModule()
    {
        if (module == nullptr) {
            return;
        }
        
        module_inst = wasm_runtime_instantiate(module, 65536, 65536, error_buf, sizeof(error_buf));
        if (module_inst != nullptr) {
            exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        }
    }

    RuntimeInitArgs init_args;
    uint8* wasm_file_buf;
    uint32 wasm_file_size;
    uint8* aot_file_buf;
    uint32 aot_file_size;
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;
    char error_buf[256];
};

TEST_F(AOTLoaderEnhancedTest, UtilityFunctions_ErrorScenarios_HandleGracefully)
{
    struct {
        const char* name;
        uint8 data[32];
        size_t size;
    } test_cases[] = {
        {"invalid_magic", {0xFF, 0xFF, 0xFF, 0xFF}, 4},
        {"truncated_header", {0x00, 0x61, 0x6f, 0x74, 0x02}, 5},
        {"invalid_version", {0x00, 0x61, 0x6f, 0x74, 0xFF, 0xFF, 0xFF, 0xFF}, 8},
        {"malformed_sections", {0x00, 0x61, 0x6f, 0x74, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF}, 10}
    };
    
    for (auto &tc : test_cases) {
        char local_error_buf[256];
        wasm_module_t invalid_module = wasm_runtime_load(
            tc.data,
            tc.size,
            local_error_buf,
            sizeof(local_error_buf)
        );

        ASSERT_EQ(nullptr, invalid_module) << "Test case: " << tc.name;
        ASSERT_GT(strlen(local_error_buf), static_cast<size_t>(0)) << "Test case: " << tc.name;
    }
}
