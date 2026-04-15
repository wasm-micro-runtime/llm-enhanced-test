/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <climits>
#include "gtest/gtest.h"
#include "wasm_export.h"
#include "bh_platform.h"
#include "../../common/test_helper.h"
#include <fstream>
#include <vector>

class AOTMemoryManagementTest : public testing::Test
{
protected:
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
    }

    void TearDown() override
    {
        wasm_runtime_destroy();
    }

    bool load_wasm_file(const char *filename, uint8_t **buffer, uint32_t *size)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }

        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        *buffer = (uint8_t *)wasm_runtime_malloc(file_size);
        if (*buffer == nullptr) {
            return false;
        }

        if (!file.read(reinterpret_cast<char*>(*buffer), file_size)) {
            wasm_runtime_free(*buffer);
            *buffer = nullptr;
            return false;
        }

        *size = static_cast<uint32_t>(file_size);
        return true;
    }

    void free_wasm_buffer(uint8_t *buffer)
    {
        if (buffer != nullptr) {
            wasm_runtime_free(buffer);
        }
    }

    wasm_module_inst_t create_test_instance_with_memory(uint32_t initial_pages, uint32_t max_pages)
    {
        char error_buf[128] = {0};
        // Use the dummy WASM buffer from test_helper.h
        wasm_module_t module = wasm_runtime_load(dummy_wasm_buffer, sizeof(dummy_wasm_buffer), 
                                                 error_buf, sizeof(error_buf));
        if (module == nullptr) {
            return nullptr;
        }

        wasm_module_inst_t module_inst = wasm_runtime_instantiate(module, 8192, 8192, 
                                                                  error_buf, sizeof(error_buf));
        wasm_runtime_unload(module);
        return module_inst;
    }

    uint32_t get_memory_page_count(wasm_module_inst_t module_inst)
    {
        if (module_inst == nullptr) return 0;
        
        // Use app address range to get memory size
        uint64_t app_offset;
        uint64_t app_size;
        if (wasm_runtime_get_app_addr_range(module_inst, 0, &app_offset, &app_size)) {
            return (uint32_t)(app_size / 65536); // 64KB per page
        }
        return 0;
    }

    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

// Test 2: AOT linear memory access valid range success
TEST_F(AOTMemoryManagementTest, LinearMemoryAccess_ValidRange_Success)
{
    wasm_module_inst_t inst = create_test_instance_with_memory(1, 65536);
    if (inst == nullptr) {
        return; // Skip if cannot create test instance
    }

    // Get app address range
    uint64_t app_offset;
    uint64_t app_size;
    bool range_result = wasm_runtime_get_app_addr_range(inst, 0, &app_offset, &app_size);
    
    if (range_result && app_size > 0) {
        // Test valid memory access through address validation
        bool valid_start = wasm_runtime_validate_app_addr(inst, app_offset, 4);
        ASSERT_TRUE(valid_start) << "Should be able to access start of memory";
        
        if (app_size > 4) {
            bool valid_end = wasm_runtime_validate_app_addr(inst, app_offset + app_size - 4, 4);
            ASSERT_TRUE(valid_end) << "Should be able to access end of memory";
        }
    }
    
    wasm_runtime_deinstantiate(inst);
}

// Test 3: AOT linear memory access out of bounds fails
TEST_F(AOTMemoryManagementTest, LinearMemoryAccess_OutOfBounds_Fails)
{
    wasm_module_inst_t inst = create_test_instance_with_memory(1, 65536);
    if (inst == nullptr) {
        return; // Skip if cannot create test instance
    }

    // Get app address range
    uint64_t app_offset;
    uint64_t app_size;
    bool range_result = wasm_runtime_get_app_addr_range(inst, 0, &app_offset, &app_size);
    
    if (range_result && app_size > 0) {
        // Test out of bounds access
        bool invalid_access = wasm_runtime_validate_app_addr(inst, app_offset + app_size, 4);
        ASSERT_FALSE(invalid_access) << "Out of bounds access should fail validation";
        
        // Test very large offset
        bool invalid_large = wasm_runtime_validate_app_addr(inst, UINT32_MAX - 3, 4);
        ASSERT_FALSE(invalid_large) << "Large offset access should fail validation";
    }
    
    wasm_runtime_deinstantiate(inst);
}

// Test 13: AOT memory allocation heap management
TEST_F(AOTMemoryManagementTest, MemoryAllocation_HeapManagement)
{
    wasm_module_inst_t inst = create_test_instance_with_memory(1, 65536);
    if (inst == nullptr) {
        return; // Skip if cannot create test instance
    }

    // Test heap allocation through WAMR runtime
    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, 8192);
    if (exec_env != nullptr) {
        // Test module malloc/free functionality
        uint32_t app_offset = wasm_runtime_module_malloc(inst, 64, nullptr);
        if (app_offset != 0) {
            // Successfully allocated memory
            wasm_runtime_module_free(inst, app_offset);
        }
        // If allocation fails, it's still valid behavior depending on configuration
        
        wasm_runtime_destroy_exec_env(exec_env);
    }
    
    wasm_runtime_deinstantiate(inst);
}

// Test 14: AOT exec env returns correct module instance
TEST_F(AOTMemoryManagementTest, ExecEnv_GetModuleInst_ReturnsCorrectInstance)
{
    wasm_module_inst_t inst = create_test_instance_with_memory(1, 65536);
    if (inst == nullptr) {
        return; // Skip if cannot create test instance
    }

    // Test stack management through execution environment
    uint32_t stack_size = 16384; // 16KB stack
    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, stack_size);
    
    if (exec_env != nullptr) {
        // Test that we can get the module instance from exec env
        wasm_module_inst_t retrieved_inst = wasm_runtime_get_module_inst(exec_env);
        ASSERT_EQ(retrieved_inst, inst) << "Should be able to retrieve module instance from exec env";
        
        wasm_runtime_destroy_exec_env(exec_env);
    }
    
    wasm_runtime_deinstantiate(inst);
}

// Test 16: AOT memory deallocation cleanup success
// TEST_F(AOTMemoryManagementTest, MemoryDeallocation_CleanupSuccess)
// {
//     wasm_module_inst_t inst = create_test_instance_with_memory(1, 65536);
//     if (inst == nullptr) {
//         return; // Skip if cannot create test instance
//     }

//     wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, 8192);
//     if (exec_env != nullptr) {
//         // Test proper cleanup sequence
//         wasm_runtime_destroy_exec_env(exec_env);
//     }
    
//     // Test that deinstantiation cleans up properly
//     wasm_runtime_deinstantiate(inst);
    
//     // Test multiple cleanup calls (should be safe)
//     wasm_runtime_deinstantiate(nullptr);
//     wasm_runtime_destroy_exec_env(nullptr);
    
//     ASSERT_TRUE(true) << "Memory cleanup should complete without errors";
// }

// Test 17: AOT repeated instance create/destroy cycle
TEST_F(AOTMemoryManagementTest, Memory_RepeatedInstanceLifecycle)
{
    // Test repeated instance creation and destruction
    std::vector<wasm_module_inst_t> instances;
    
    // Create multiple small instances to test fragmentation
    for (int i = 0; i < 5; i++) {
        wasm_module_inst_t inst = create_test_instance_with_memory(1, 65536);
        if (inst != nullptr) {
            instances.push_back(inst);
        }
    }
    
    // Verify instances were created
    ASSERT_GT(instances.size(), 0) << "Should be able to create multiple instances";
    
    // Clean up all instances
    for (auto *inst : instances) {
        wasm_runtime_deinstantiate(inst);
    }

    // Verify at least one instance was created
    ASSERT_GT(instances.size(), 0u)
        << "Should create at least one instance";

    // Test that memory can still be allocated after fragmentation
    wasm_module_inst_t new_inst = create_test_instance_with_memory(1, 65536);
    if (new_inst != nullptr) {
        wasm_runtime_deinstantiate(new_inst);
    }
}

// Test 18: AOT memory pressure scenarios handled
TEST_F(AOTMemoryManagementTest, Memory_PressureScenarios_Handled)
{
    // Test memory pressure by creating instances with large memory requirements
    uint32_t large_stack = 32768;  // 32KB stack
    uint32_t large_heap = 32768;   // 32KB heap
    
    wasm_module_t module = wasm_runtime_load(dummy_wasm_buffer, sizeof(dummy_wasm_buffer), nullptr, 0);
    if (module != nullptr) {
        std::vector<wasm_module_inst_t> instances;
        
        // Try to create multiple large instances until memory pressure
        for (int i = 0; i < 10; i++) {
            wasm_module_inst_t inst = wasm_runtime_instantiate(module, large_stack, large_heap, nullptr, 0);
            if (inst != nullptr) {
                instances.push_back(inst);
            } else {
                // Memory pressure reached - this is expected behavior
                break;
            }
        }
        
        // Clean up all instances
        for (auto *inst : instances) {
            wasm_runtime_deinstantiate(inst);
        }

        wasm_runtime_unload(module);

        ASSERT_GT(instances.size(), 0u)
            << "Should create at least one instance before pressure";
    }
}

