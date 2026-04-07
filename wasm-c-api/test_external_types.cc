/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_c_api.h"

class ExternalTypeSystemTest : public testing::Test
{
protected:
    void SetUp() override
    {
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
    }

    void TearDown() override
    {
        if (store != nullptr) {
            wasm_store_delete(store);
            store = nullptr;
        }
        if (engine != nullptr) {
            wasm_engine_delete(engine);
            engine = nullptr;
        }
    }

    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;
};

TEST_F(ExternalTypeSystemTest, ExternType_ConstConversion_WorksCorrectly)
{
    // Create table type
    wasm_valtype_t* valtype = wasm_valtype_new_funcref();
    ASSERT_NE(nullptr, valtype);
    wasm_limits_t limits = { 1, 10 };
    wasm_tabletype_t* tabletype = wasm_tabletype_new(valtype, &limits);
    ASSERT_NE(nullptr, tabletype);

    // Test const conversion
    const wasm_externtype_t* const_externtype = wasm_tabletype_as_externtype_const(tabletype);
    ASSERT_NE(nullptr, const_externtype);
    ASSERT_EQ(WASM_EXTERN_TABLE, wasm_externtype_kind(const_externtype));

    // Convert back using const version
    const wasm_tabletype_t* const_tabletype = wasm_externtype_as_tabletype_const(const_externtype);
    ASSERT_NE(nullptr, const_tabletype);

    // Verify data integrity
    const wasm_valtype_t* element = wasm_tabletype_element(const_tabletype);
    ASSERT_NE(nullptr, element);
    ASSERT_EQ(WASM_FUNCREF, wasm_valtype_kind(element));

    wasm_tabletype_delete(tabletype);
}

// External Type Error Handling Tests

TEST_F(ExternalTypeSystemTest, ExternType_NullInput_HandlesGracefully)
{
    // Test null conversion attempts
    wasm_functype_t* functype = wasm_externtype_as_functype(nullptr);
    ASSERT_EQ(nullptr, functype);

    wasm_globaltype_t* globaltype = wasm_externtype_as_globaltype(nullptr);
    ASSERT_EQ(nullptr, globaltype);

    wasm_tabletype_t* tabletype = wasm_externtype_as_tabletype(nullptr);
    ASSERT_EQ(nullptr, tabletype);

    wasm_memorytype_t* memorytype = wasm_externtype_as_memorytype(nullptr);
    ASSERT_EQ(nullptr, memorytype);
}

TEST_F(ExternalTypeSystemTest, ExternType_ComplexTypeSystem_IntegrationTest)
{
    // Create multiple external types and verify they work together
    
    // Function type: (i32, f64) -> i64
    wasm_valtype_t* func_params[] = { wasm_valtype_new_i32(), wasm_valtype_new_f64() };
    wasm_valtype_t* func_results[] = { wasm_valtype_new_i64() };
    wasm_valtype_vec_t params, results;
    wasm_valtype_vec_new(&params, 2, func_params);
    wasm_valtype_vec_new(&results, 1, func_results);
    wasm_functype_t* functype = wasm_functype_new(&params, &results);
    
    // Global type: var f32
    wasm_valtype_t* global_valtype = wasm_valtype_new_f32();
    wasm_globaltype_t* globaltype = wasm_globaltype_new(global_valtype, WASM_VAR);
    
    // Table type: funcref[5..50]
    wasm_valtype_t* table_valtype = wasm_valtype_new_funcref();
    wasm_limits_t table_limits = { 5, 50 };
    wasm_tabletype_t* tabletype = wasm_tabletype_new(table_valtype, &table_limits);
    
    // Memory type: [2..256]
    wasm_limits_t memory_limits = { 2, 256 };
    wasm_memorytype_t* memorytype = wasm_memorytype_new(&memory_limits);

    // Convert all to external types
    wasm_externtype_t* extern_types[] = {
        wasm_functype_as_externtype(functype),
        wasm_globaltype_as_externtype(globaltype),
        wasm_tabletype_as_externtype(tabletype),
        wasm_memorytype_as_externtype(memorytype)
    };

    // Verify all conversions succeeded
    for (wasm_externtype_t* et : extern_types) {
        ASSERT_NE(nullptr, et);
    }

    // Verify kinds are correct and unique
    wasm_externkind_t kinds[] = {
        wasm_externtype_kind(extern_types[0]),
        wasm_externtype_kind(extern_types[1]),
        wasm_externtype_kind(extern_types[2]),
        wasm_externtype_kind(extern_types[3])
    };
    
    ASSERT_EQ(WASM_EXTERN_FUNC, kinds[0]);
    ASSERT_EQ(WASM_EXTERN_GLOBAL, kinds[1]);
    ASSERT_EQ(WASM_EXTERN_TABLE, kinds[2]);
    ASSERT_EQ(WASM_EXTERN_MEMORY, kinds[3]);

    // Verify each kind is unique
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = i + 1; j < 4; ++j) {
            ASSERT_NE(kinds[i], kinds[j]);
        }
    }

    // Verify correct type conversions work
    wasm_functype_t* converted_func = wasm_externtype_as_functype(extern_types[0]);
    ASSERT_NE(nullptr, converted_func);
    ASSERT_EQ(2, wasm_functype_params(converted_func)->size);
    ASSERT_EQ(1, wasm_functype_results(converted_func)->size);

    wasm_globaltype_t* converted_global = wasm_externtype_as_globaltype(extern_types[1]);
    ASSERT_NE(nullptr, converted_global);
    ASSERT_EQ(WASM_VAR, wasm_globaltype_mutability(converted_global));

    wasm_tabletype_t* converted_table = wasm_externtype_as_tabletype(extern_types[2]);
    ASSERT_NE(nullptr, converted_table);
    ASSERT_EQ(5, wasm_tabletype_limits(converted_table)->min);
    ASSERT_EQ(50, wasm_tabletype_limits(converted_table)->max);

    wasm_memorytype_t* converted_memory = wasm_externtype_as_memorytype(extern_types[3]);
    ASSERT_NE(nullptr, converted_memory);
    ASSERT_EQ(2, wasm_memorytype_limits(converted_memory)->min);
    ASSERT_EQ(256, wasm_memorytype_limits(converted_memory)->max);

    // Clean up
    wasm_functype_delete(functype);
    wasm_globaltype_delete(globaltype);
    wasm_tabletype_delete(tabletype);
    wasm_memorytype_delete(memorytype);
}
