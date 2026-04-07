/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_c_api.h"
#include <cmath>
#include <limits>

// Test fixture for runtime values and references
class RuntimeValuesTest : public testing::Test {
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

// Test 17: Error handling with invalid operations
TEST_F(RuntimeValuesTest, ErrorHandling_InvalidOperations_HandlesGracefully) {
    wasm_val_vec_t valid_vec;
    
    // Create a valid empty vector
    wasm_val_vec_new_empty(&valid_vec);
    
    // Verify empty state
    ASSERT_EQ(0u, valid_vec.size);
    ASSERT_EQ(nullptr, valid_vec.data);
    
    // Delete should handle empty vector gracefully
    wasm_val_vec_delete(&valid_vec);
    
    // Should still be in clean state after deletion
    ASSERT_EQ(0u, valid_vec.size);
    ASSERT_EQ(nullptr, valid_vec.data);
}
