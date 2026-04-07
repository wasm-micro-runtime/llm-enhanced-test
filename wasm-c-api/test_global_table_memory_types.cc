/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_c_api.h"

class GlobalTableMemoryTypesTest : public testing::Test {
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

// Type Validation Tests
TEST_F(GlobalTableMemoryTypesTest, GlobalType_NullValtype_HandlesGracefully) {
    // This test checks behavior with null valtype - should handle gracefully
    wasm_globaltype_t* globaltype = wasm_globaltype_new(nullptr, WASM_CONST);
    ASSERT_EQ(globaltype, nullptr);
    if (globaltype != nullptr) {
        wasm_globaltype_delete(globaltype);
    }
}
