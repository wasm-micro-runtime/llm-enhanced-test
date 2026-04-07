/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include "wasm_c_api.h"

class ImportExportTypesTest : public testing::Test
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
        }
        if (engine != nullptr) {
            wasm_engine_delete(engine);
        }
    }

    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;

    // Helper to create a simple function type
    wasm_functype_t* create_simple_functype()
    {
        wasm_valtype_vec_t params, results;
        wasm_valtype_vec_new_empty(&params);
        wasm_valtype_vec_new_uninitialized(&results, 1);
        results.data[0] = wasm_valtype_new(WASM_I32);
        return wasm_functype_new(&params, &results);
    }

    // Helper to create a simple global type
    wasm_globaltype_t* create_simple_globaltype()
    {
        wasm_valtype_t* valtype = wasm_valtype_new(WASM_I32);
        return wasm_globaltype_new(valtype, WASM_CONST);
    }

    // Helper to create a name
    wasm_name_t* create_name(const char* str)
    {
        auto* name = new wasm_name_t;
        wasm_name_new_from_string(name, str);
        return name;
    }
};

TEST_F(ImportExportTypesTest, ImportType_IsLinked_ReturnsCorrectStatus)
{
    wasm_name_t* module_name = create_name("test_module");
    wasm_name_t* import_name = create_name("test_function");
    wasm_functype_t* functype = create_simple_functype();
    wasm_externtype_t* externtype = wasm_functype_as_externtype(functype);
    
    wasm_importtype_t* importtype = wasm_importtype_new(module_name, import_name, externtype);
    
    // Initially should not be linked
    bool is_linked = wasm_importtype_is_linked(importtype);
    ASSERT_FALSE(is_linked);
    
    wasm_importtype_delete(importtype);
}

TEST_F(ImportExportTypesTest, ImportType_CreateWithNullExternType_HandlesGracefully)
{
    wasm_name_t* module_name = create_name("test_module");
    wasm_name_t* import_name = create_name("test_function");
    
    wasm_importtype_t* importtype = wasm_importtype_new(module_name, import_name, nullptr);

    ASSERT_EQ(importtype, nullptr);
    wasm_name_delete(module_name);
    delete module_name;
    wasm_name_delete(import_name);
    delete import_name;
}

TEST_F(ImportExportTypesTest, ExportType_CreateWithNullExternType_HandlesGracefully)
{
    wasm_name_t* export_name = create_name("exported_function");
    
    wasm_exporttype_t* exporttype = wasm_exporttype_new(export_name, nullptr);

    ASSERT_EQ(exporttype, nullptr);
    wasm_name_delete(export_name);
    delete export_name;
}
