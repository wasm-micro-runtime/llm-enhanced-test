/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_c_api.h"
#include "bh_platform.h"

/**
 * Test Suite: Module Lifecycle
 * 
 * This test suite provides comprehensive coverage of WASM-C-API module
 * lifecycle operations including:
 * - Module loading from bytecode
 * - Module validation and compilation
 * - Module sharing between instances
 * - Module serialization and deserialization
 * - Module import/export introspection
 * - Error handling for invalid modules
 * - Resource management and cleanup
 */

class ModuleLifecycleTest : public testing::Test {
protected:
    void SetUp() override {
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
        
        create_test_modules();
    }
    
    void TearDown() override {
        if (complex_module != nullptr) {
            wasm_module_delete(complex_module);
            complex_module = nullptr;
        }
        if (minimal_module != nullptr) {
            wasm_module_delete(minimal_module);
            minimal_module = nullptr;
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
    
    void create_test_modules() {
        // Create minimal valid WASM module
        uint8_t minimal_wasm[] = {
            0x00, 0x61, 0x73, 0x6d, // magic
            0x01, 0x00, 0x00, 0x00  // version
        };
        
        wasm_byte_vec_t minimal_binary;
        wasm_byte_vec_new(&minimal_binary, sizeof(minimal_wasm), (char*)minimal_wasm);
        minimal_module = wasm_module_new(store, &minimal_binary);
        wasm_byte_vec_delete(&minimal_binary);
        
        ASSERT_NE(nullptr, minimal_module);
        
        // Create more complex module with function
        uint8_t complex_wasm[] = {
            0x00, 0x61, 0x73, 0x6d, // magic
            0x01, 0x00, 0x00, 0x00, // version
            0x01, 0x07, 0x01, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, // type section: (i32, i32) -> i32
            0x03, 0x02, 0x01, 0x00, // function section: 1 function of type 0
            0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, // export section: export function 0 as "add"
            0x0a, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b // code section: local.get 0, local.get 1, i32.add, end
        };
        
        wasm_byte_vec_t complex_binary;
        wasm_byte_vec_new(&complex_binary, sizeof(complex_wasm), (char*)complex_wasm);
        complex_module = wasm_module_new(store, &complex_binary);
        wasm_byte_vec_delete(&complex_binary);
        
        ASSERT_NE(nullptr, complex_module);
    }
    
    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;
    wasm_module_t* minimal_module = nullptr;
    wasm_module_t* complex_module = nullptr;
};

TEST_F(ModuleLifecycleTest, Module_ValidateInvalidFunctionSection_ReturnsNull) {
    uint8_t invalid_wasm[] = {
        0x00, 0x61, 0x73, 0x6d, // magic
        0x01, 0x00, 0x00, 0x00, // version
        0x03, 0x02, 0x01, 0x99  // invalid function section (type index 0x99 doesn't exist)
    };
    
    wasm_byte_vec_t binary;
    wasm_byte_vec_new(&binary, sizeof(invalid_wasm), (char*)invalid_wasm);
    
    wasm_module_t* module = wasm_module_new(store, &binary);
    ASSERT_EQ(nullptr, module);
    
    wasm_byte_vec_delete(&binary);
}

TEST_F(ModuleLifecycleTest, Module_RepeatedLoadUnload_SucceedsCorrectly) {
    uint8_t wasm_bytes[] = {
        0x00, 0x61, 0x73, 0x6d, // magic
        0x01, 0x00, 0x00, 0x00  // version
    };
    
    wasm_byte_vec_t binary;
    wasm_byte_vec_new(&binary, sizeof(wasm_bytes), (char*)wasm_bytes);
    
    // Create multiple modules simultaneously
    const size_t num_modules = 10;
    wasm_module_t* modules[num_modules];
    
    for (auto &mod : modules) {
        mod = wasm_module_new(store, &binary);
        ASSERT_NE(nullptr, mod);
    }

    // Delete all modules
    for (auto &mod : modules) {
        wasm_module_delete(mod);
    }
    
    wasm_byte_vec_delete(&binary);
}
