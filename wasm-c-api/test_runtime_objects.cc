/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_c_api.h"
#include "bh_platform.h"

class WasmRuntimeObjectsTest : public testing::Test {
protected:
    void SetUp() override {
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
    }
    
    void TearDown() override {
        if (store != nullptr) wasm_store_delete(store);
        if (engine != nullptr) wasm_engine_delete(engine);
    }
    
    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;
};

TEST_F(WasmRuntimeObjectsTest, Global_SetWithoutInstance_ReturnsSilently) {
    wasm_valtype_t* valtype = wasm_valtype_new(WASM_F64);
    ASSERT_NE(nullptr, valtype);
    
    wasm_globaltype_t* globaltype = wasm_globaltype_new(valtype, WASM_CONST);
    ASSERT_NE(nullptr, globaltype);
    
    wasm_val_t initial_val;
    initial_val.kind = WASM_F64;
    initial_val.of.f64 = 3.14159;
    
    wasm_global_t* global = wasm_global_new(store, globaltype, &initial_val);
    ASSERT_NE(nullptr, global);
    
    // Attempt to update const global (should not crash)
    wasm_val_t new_val;
    new_val.kind = WASM_F64;
    new_val.of.f64 = 2.71828;
    wasm_global_set(global, &new_val);
    
    // Verify global remains const by checking its type
    wasm_globaltype_t* inspected_type = wasm_global_type(global);
    ASSERT_NE(nullptr, inspected_type);
    
    wasm_mutability_t mutability = wasm_globaltype_mutability(inspected_type);
    ASSERT_EQ(WASM_CONST, mutability);
    
    wasm_globaltype_delete(inspected_type);
    wasm_global_delete(global);
    wasm_globaltype_delete(globaltype);
}

TEST_F(WasmRuntimeObjectsTest, Table_GetSetWithoutInstance_ReturnsNullAndFalse) {
    wasm_valtype_t* element_type = wasm_valtype_new(WASM_FUNCREF);
    ASSERT_NE(nullptr, element_type);
    
    wasm_limits_t limits = { .min = 3, .max = 5 };
    wasm_tabletype_t* tabletype = wasm_tabletype_new(element_type, &limits);
    ASSERT_NE(nullptr, tabletype);
    
    wasm_table_t* table = wasm_table_new(store, tabletype, nullptr);
    ASSERT_NE(nullptr, table);

    // Size is 0 because no backing module instance
    wasm_table_size_t size = wasm_table_size(table);
    ASSERT_EQ(0u, size);

    // Get element at index 0 - returns nullptr because no backing module instance
    wasm_ref_t* element = wasm_table_get(table, 0);
    ASSERT_EQ(nullptr, element);

    // Set element at index 0 - returns false because inst_comm_rt is NULL (no module instance)
    bool set_result = wasm_table_set(table, 0, nullptr);
    ASSERT_FALSE(set_result);

    wasm_table_delete(table);
    wasm_tabletype_delete(tabletype);
}

TEST_F(WasmRuntimeObjectsTest, Table_Grow_IsUnsupportedByHost_ReturnsFalse) {
    wasm_valtype_t* element_type = wasm_valtype_new(WASM_FUNCREF);
    ASSERT_NE(nullptr, element_type);
    
    wasm_limits_t limits = { .min = 2, .max = 4 };
    wasm_tabletype_t* tabletype = wasm_tabletype_new(element_type, &limits);
    ASSERT_NE(nullptr, tabletype);
    
    wasm_table_t* table = wasm_table_new(store, tabletype, nullptr);
    ASSERT_NE(nullptr, table);

    // Try to grow - wasm_table_grow is unsupported for host, always returns false
    bool grow_result = wasm_table_grow(table, 5, nullptr);
    ASSERT_FALSE(grow_result);
    
    wasm_table_delete(table);
    wasm_tabletype_delete(tabletype);
}

// Object Host Info Management Tests

TEST_F(WasmRuntimeObjectsTest, Global_HostInfo_ManagesCorrectly) {
    wasm_valtype_t* valtype = wasm_valtype_new(WASM_I32);
    ASSERT_NE(nullptr, valtype);
    
    wasm_globaltype_t* globaltype = wasm_globaltype_new(valtype, WASM_VAR);
    ASSERT_NE(nullptr, globaltype);
    
    wasm_val_t initial_val;
    initial_val.kind = WASM_I32;
    initial_val.of.i32 = 42;
    
    wasm_global_t* global = wasm_global_new(store, globaltype, &initial_val);
    ASSERT_NE(nullptr, global);
    
    // Set host info
    int host_data = 12345;
    wasm_global_set_host_info(global, &host_data);
    
    // Get host info
    void* retrieved_info = wasm_global_get_host_info(global);
    ASSERT_EQ(&host_data, retrieved_info);
    ASSERT_EQ(12345, *(int*)retrieved_info);
    
    wasm_global_delete(global);
    wasm_globaltype_delete(globaltype);
}

TEST_F(WasmRuntimeObjectsTest, Table_HostInfo_ManagesCorrectly) {
    wasm_valtype_t* element_type = wasm_valtype_new(WASM_FUNCREF);
    ASSERT_NE(nullptr, element_type);
    
    wasm_limits_t limits = { .min = 2, .max = 4 };
    wasm_tabletype_t* tabletype = wasm_tabletype_new(element_type, &limits);
    ASSERT_NE(nullptr, tabletype);
    
    wasm_table_t* table = wasm_table_new(store, tabletype, nullptr);
    ASSERT_NE(nullptr, table);

    // Set host info
    const char* host_name = "test_table";
    wasm_table_set_host_info(table, (void*)host_name);
    
    // Get host info
    void* retrieved_info = wasm_table_get_host_info(table);
    ASSERT_EQ(host_name, (const char*)retrieved_info);
    ASSERT_STREQ("test_table", (const char*)retrieved_info);
    
    wasm_table_delete(table);
    wasm_tabletype_delete(tabletype);
}

TEST_F(WasmRuntimeObjectsTest, Memory_HostInfo_ManagesCorrectly) {
    wasm_limits_t limits = { .min = 1, .max = 3 };
    wasm_memorytype_t* memorytype = wasm_memorytype_new(&limits);
    ASSERT_NE(nullptr, memorytype);
    
    wasm_memory_t* memory = wasm_memory_new(store, memorytype);
    ASSERT_NE(nullptr, memory);
    
    // Set host info
    double host_value = 3.14159;
    wasm_memory_set_host_info(memory, &host_value);
    
    // Get host info
    void* retrieved_info = wasm_memory_get_host_info(memory);
    ASSERT_EQ(&host_value, retrieved_info);
    ASSERT_DOUBLE_EQ(3.14159, *(double*)retrieved_info);
    
    wasm_memory_delete(memory);
    wasm_memorytype_delete(memorytype);
}
