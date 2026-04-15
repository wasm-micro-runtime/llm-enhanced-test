/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <vector>
#include <algorithm>

#include "bh_platform.h"
#include "wasm_c_api.h"
#include "wasm_c_api_internal.h"

#ifndef own
#define own
#endif

class FunctionTypeTest : public testing::Test {
protected:
    void SetUp() override {
        bh_log_set_verbose_level(5);
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
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
    }
    
    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;
};

// Test 11: Function type null handling
TEST_F(FunctionTypeTest, FunctionType_NullHandling_WorksCorrectly) {
    wasm_valtype_vec_t empty_params = { 0 };
    wasm_valtype_vec_t empty_results = { 0 };
    wasm_valtype_vec_new_empty(&empty_params);
    wasm_valtype_vec_new_empty(&empty_results);
    
    wasm_functype_t* func_type = wasm_functype_new(&empty_params, &empty_results);
    ASSERT_NE(nullptr, func_type);
    
    // Test delete on valid function type
    wasm_functype_delete(func_type);
    
    // Test delete on null (should not crash)
    wasm_functype_delete(nullptr);
}
