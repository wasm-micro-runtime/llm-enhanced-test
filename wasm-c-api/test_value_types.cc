/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <vector>
#include <memory>

#include "bh_platform.h"
#include "wasm_c_api.h"
#include "wasm_c_api_internal.h"

#ifndef own
#define own
#endif

class ValueTypeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bh_log_set_verbose_level(5);
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
    }

    void TearDown() override
    {
        if (store != nullptr) wasm_store_delete(store);
        if (engine != nullptr) wasm_engine_delete(engine);
    }

    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;
};

TEST_F(ValueTypeTest, ReferenceTypes_Validation_WorksCorrectly)
{
    wasm_valtype_t* funcref_type = wasm_valtype_new_funcref();
    
    ASSERT_NE(nullptr, funcref_type);
    ASSERT_EQ(WASM_FUNCREF, wasm_valtype_kind(funcref_type));
    
    wasm_valtype_delete(funcref_type);
    
    // Test anyref if available
    wasm_valtype_t* anyref_type = wasm_valtype_new_anyref();
    if (anyref_type != nullptr) {
        ASSERT_EQ(WASM_EXTERNREF, wasm_valtype_kind(anyref_type));
        wasm_valtype_delete(anyref_type);
    }
}

TEST_F(ValueTypeTest, ValueCopy_NullDestination_HandlesGracefully)
{
    wasm_val_t val = { 0 };
    val.kind = WASM_I32;
    val.of.i32 = 42;
    
    // Copying to null should not crash; source value must remain unmodified
    wasm_val_copy(nullptr, &val);
    ASSERT_EQ(WASM_I32, val.kind);
    ASSERT_EQ(42, val.of.i32);
}
