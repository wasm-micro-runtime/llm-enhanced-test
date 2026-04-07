/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_c_api.h"
#include "test_helper.h"

class WasmExternTypeTest : public testing::Test {
protected:
    void SetUp() override {
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
        runtime = std::make_unique<WAMRRuntimeRAII<512 * 1024>>();
    }

    void TearDown() override {
        if (store != nullptr) {
            wasm_store_delete(store);
            store = nullptr;
        }
        if (engine != nullptr) {
            wasm_engine_delete(engine);
            engine = nullptr;
        }
        runtime.reset();
    }

    // Helper to create a wasm_func_t for testing
    wasm_func_t* create_test_func(wasm_functype_t* functype) {
        // Create a dummy function that does nothing
        return wasm_func_new(store, functype, 
            [](const wasm_val_vec_t* args, wasm_val_vec_t* results) -> wasm_trap_t* {
                return nullptr;
            });
    }

    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;
    std::unique_ptr<WAMRRuntimeRAII<512 * 1024>> runtime;
};

// Test NULL input case - covers line: if (!external) return NULL;
TEST_F(WasmExternTypeTest, wasm_extern_type_NullInput_ReturnsNull) {
    wasm_externtype_t* result = wasm_extern_type(nullptr);
    ASSERT_EQ(nullptr, result);
}

// Test all extern kinds in one comprehensive test
TEST_F(WasmExternTypeTest, wasm_extern_type_AllExternKinds_ReturnCorrectTypes) {
    // Test all four extern kinds to ensure full branch coverage
    
    // 1. Function extern
    {
        wasm_valtype_vec_t params, results;
        wasm_valtype_vec_new_empty(&params);
        wasm_valtype_vec_new_empty(&results);
        wasm_functype_t* functype = wasm_functype_new(&params, &results);
        wasm_func_t* func = create_test_func(functype);
        wasm_extern_t* external = wasm_func_as_extern(func);
        wasm_externtype_t* externtype = wasm_extern_type(external);
        
        ASSERT_NE(nullptr, externtype);
        ASSERT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(externtype));
        
        wasm_func_delete(func);
        wasm_functype_delete(functype);
    }
    
    // 2. Global extern
    {
        wasm_valtype_t* valtype = wasm_valtype_new_i32();
        wasm_globaltype_t* globaltype = wasm_globaltype_new(valtype, WASM_VAR);
        wasm_val_t value = WASM_I32_VAL(100);
        wasm_global_t* global = wasm_global_new(store, globaltype, &value);
        wasm_extern_t* external = wasm_global_as_extern(global);
        wasm_externtype_t* externtype = wasm_extern_type(external);
        
        ASSERT_NE(nullptr, externtype);
        ASSERT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtype));
        
        wasm_global_delete(global);
        wasm_globaltype_delete(globaltype);
    }
    
    // 3. Memory extern
    {
        wasm_limits_t limits = { 2, 20 };
        wasm_memorytype_t* memorytype = wasm_memorytype_new(&limits);
        wasm_memory_t* memory = wasm_memory_new(store, memorytype);
        wasm_extern_t* external = wasm_memory_as_extern(memory);
        wasm_externtype_t* externtype = wasm_extern_type(external);
        
        ASSERT_NE(nullptr, externtype);
        ASSERT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(externtype));
        
        wasm_memory_delete(memory);
        wasm_memorytype_delete(memorytype);
    }
    
    // 4. Table extern
    {
        wasm_valtype_t* valtype = wasm_valtype_new_funcref();
        wasm_limits_t limits = { 5, 25 };
        wasm_tabletype_t* tabletype = wasm_tabletype_new(valtype, &limits);
        wasm_table_t* table = wasm_table_new(store, tabletype, nullptr);
        wasm_extern_t* external = wasm_table_as_extern(table);
        wasm_externtype_t* externtype = wasm_extern_type(external);
        
        ASSERT_NE(nullptr, externtype);
        ASSERT_EQ(WASM_EXTERN_TABLE, wasm_externtype_kind(externtype));
        
        wasm_table_delete(table);
        wasm_tabletype_delete(tabletype);
    }
}

// Test const extern handling
TEST_F(WasmExternTypeTest, wasm_extern_type_ConstExtern_HandlesCorrectly) {
    // Create memory extern
    wasm_limits_t limits = { 1, 5 };
    wasm_memorytype_t* memorytype = wasm_memorytype_new(&limits);
    wasm_memory_t* memory = wasm_memory_new(store, memorytype);
    const wasm_extern_t* const_external = wasm_memory_as_extern_const(memory);
    ASSERT_NE(nullptr, const_external);

    // Test wasm_extern_type with const extern
    wasm_externtype_t* externtype = wasm_extern_type(const_external);
    ASSERT_NE(nullptr, externtype);
    ASSERT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(externtype));

    // Verify we can still get the memory type back
    wasm_memorytype_t* retrieved_memorytype = wasm_externtype_as_memorytype(externtype);
    ASSERT_NE(nullptr, retrieved_memorytype);
    
    const wasm_limits_t* retrieved_limits = wasm_memorytype_limits(retrieved_memorytype);
    ASSERT_NE(nullptr, retrieved_limits);
    ASSERT_EQ(1, retrieved_limits->min);
    ASSERT_EQ(5, retrieved_limits->max);

    // Cleanup
    wasm_memory_delete(memory);
    wasm_memorytype_delete(memorytype);
}
