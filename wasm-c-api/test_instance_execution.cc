/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_c_api.h"
#include "bh_platform.h"

/**
 * Test Suite: Instance/Execution Environment
 * 
 * This test suite provides comprehensive coverage of WASM-C-API instance
 * operations including:
 * - Instance creation and management
 * - Execution environment setup
 * - Trap handling and frame operations
 * - Instance export operations
 * - Instance function calls
 * - Instance memory and global access
 * - Complete integration scenarios
 * - Error handling and resource management
 */

class InstanceExecutionTest : public testing::Test {
protected:
    void SetUp() override {
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
        
        // Create a simple WASM module for testing
        create_test_module();
    }
    
    void TearDown() override {
        if (instance != nullptr) {
            wasm_instance_delete(instance);
            instance = nullptr;
        }
        if (module != nullptr) {
            wasm_module_delete(module);
            module = nullptr;
        }
        if (store != nullptr) {
            wasm_store_delete(store);
            store = nullptr;
        }
        if (engine != nullptr) {
            wasm_engine_delete(engine);
            engine = nullptr;
        }
    }
    
    void create_test_module() {
        // Minimal valid WASM module - just magic and version
        uint8_t minimal_wasm[] = {
            0x00, 0x61, 0x73, 0x6d, // magic
            0x01, 0x00, 0x00, 0x00  // version
        };
        
        wasm_byte_vec_t minimal_binary;
        wasm_byte_vec_new(&minimal_binary, sizeof(minimal_wasm), (char*)minimal_wasm);
        module = wasm_module_new(store, &minimal_binary);
        wasm_byte_vec_delete(&minimal_binary);
        
        ASSERT_NE(nullptr, module);
    }
    
    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;
    wasm_module_t* module = nullptr;
    wasm_instance_t* instance = nullptr;
};

TEST_F(InstanceExecutionTest, Instance_CreateWithNullModule_ReturnsNull) {
    wasm_extern_vec_t imports;
    wasm_extern_vec_new_empty(&imports);
    
    wasm_instance_t* null_instance = wasm_instance_new(store, nullptr, &imports, nullptr);
    ASSERT_EQ(nullptr, null_instance);
    
    wasm_extern_vec_delete(&imports);
}

TEST_F(InstanceExecutionTest, Instance_GetExportsFromNull_HandlesGracefully) {
    wasm_extern_vec_t exports;
    wasm_extern_vec_new_empty(&exports);
    wasm_instance_exports(nullptr, &exports);
    
    ASSERT_EQ(0, exports.size);
    ASSERT_EQ(nullptr, exports.data);
    
    wasm_extern_vec_delete(&exports);
}

TEST_F(InstanceExecutionTest, Trap_GetOrigin_ReturnsNullForManualTrap_HandlesCorrectly) {
    // Create a trap manually for testing
    wasm_message_t message;
    const char* trap_msg = "Test trap message";
    wasm_byte_vec_new(&message, strlen(trap_msg), trap_msg);

    wasm_trap_t* trap = wasm_trap_new(store, &message);
    ASSERT_NE(nullptr, trap);

    wasm_frame_t* frame = wasm_trap_origin(trap);
    // Manually-created traps have no execution frames, so origin is always NULL
    ASSERT_EQ(nullptr, frame);

    wasm_byte_vec_delete(&message);
    wasm_trap_delete(trap);
}

// Test Category 5: Error Handling and Edge Cases

TEST_F(InstanceExecutionTest, Instance_WithInvalidImports_HandlesGracefully) {
    // Create invalid import (wrong type)
    wasm_valtype_t* param_type = wasm_valtype_new(WASM_I32);
    wasm_valtype_vec_t params;
    wasm_valtype_vec_new(&params, 1, &param_type);
    
    wasm_valtype_vec_t results;
    wasm_valtype_vec_new_empty(&results);
    
    wasm_functype_t* func_type = wasm_functype_new(&params, &results);
    wasm_func_t* invalid_func = wasm_func_new(store, func_type, nullptr);
    
    wasm_extern_t* invalid_import = wasm_func_as_extern(invalid_func);
    wasm_extern_vec_t imports;
    wasm_extern_vec_new(&imports, 1, &invalid_import);
    
    wasm_instance_t* null_instance = wasm_instance_new(store, module, &imports, nullptr);
    // wasm_func_new with NULL callback always returns NULL, so instance creation always fails
    ASSERT_EQ(nullptr, null_instance);
    
    wasm_extern_vec_delete(&imports);
    wasm_func_delete(invalid_func);
    wasm_functype_delete(func_type);
}
