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

class VectorTest : public ::testing::Test
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

// Function Type Vector Tests
TEST_F(VectorTest, FunctypeVec_MultipleFunctionTypes_CreatesCorrectly)
{
    // Create different function types
    wasm_functype_t* func_types[3] = {
        wasm_functype_new_0_0(),
        wasm_functype_new_1_0(wasm_valtype_new_i32()),
        wasm_functype_new_0_1(wasm_valtype_new_f64())
    };
    
    wasm_functype_vec_t vec = { 0 };
    wasm_functype_vec_new(&vec, 3, func_types);
    
    ASSERT_EQ(3, vec.size);
    ASSERT_NE(nullptr, vec.data);
    
    // Verify function signatures
    const wasm_valtype_vec_t* params0 = wasm_functype_params(vec.data[0]);
    const wasm_valtype_vec_t* results0 = wasm_functype_results(vec.data[0]);
    ASSERT_EQ(0, params0->size);
    ASSERT_EQ(0, results0->size);
    
    const wasm_valtype_vec_t* params1 = wasm_functype_params(vec.data[1]);
    ASSERT_EQ(1, params1->size);
    ASSERT_EQ(WASM_I32, wasm_valtype_kind(params1->data[0]));
    
    const wasm_valtype_vec_t* results2 = wasm_functype_results(vec.data[2]);
    ASSERT_EQ(1, results2->size);
    ASSERT_EQ(WASM_F64, wasm_valtype_kind(results2->data[0]));
    
    wasm_functype_vec_delete(&vec);
}

// Edge Cases and Error Handling
TEST_F(VectorTest, ByteVec_VeryLargeSize_HandlesAppropriately)
{
    wasm_byte_vec_t byte_vec = { 0 };
    const size_t very_large_size = SIZE_MAX / 2; // Likely to fail allocation
    
    // This may fail to allocate, which is acceptable
    wasm_byte_vec_new_uninitialized(&byte_vec, very_large_size);
    
    // If allocation succeeded, verify basic properties; otherwise verify graceful failure
    if (byte_vec.data != nullptr) {
        ASSERT_EQ(very_large_size, byte_vec.size);
        wasm_byte_vec_delete(&byte_vec);
    }
    else {
        ASSERT_EQ(static_cast<size_t>(0), byte_vec.num_elems);
    }
}
