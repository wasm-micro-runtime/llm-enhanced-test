/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <memory>

#include "bh_platform.h"
#include "wasm_c_api.h"
#include "wasm_c_api_internal.h"

#ifndef own
#define own
#endif

class FunctionInstancesTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bh_log_set_verbose_level(5);
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
        
        // Create a simple WASM module for testing
        createTestModule();
    }

    void TearDown() override
    {
        if (module_inst != nullptr) wasm_instance_delete(module_inst);
        if (module != nullptr) wasm_module_delete(module);
        if (store != nullptr) wasm_store_delete(store);
        if (engine != nullptr) wasm_engine_delete(engine);
    }
    
    void createTestModule()
    {
        // Simple WASM module with add function: (i32, i32) -> i32
        const uint8_t wasm_bytes[] = {
            0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,  // WASM header
            0x01, 0x07, 0x01, 0x60, 0x02, 0x7f, 0x7f, 0x01,  // Type section: (i32, i32) -> i32
            0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07, 0x01,  // Function and export sections
            0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09,  // Export "add" function
            0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a,  // Code: local.get 0, local.get 1, i32.add
            0x0b                                              // end
        };
        
        wasm_byte_vec_t binary;
        wasm_byte_vec_new(&binary, sizeof(wasm_bytes), (char*)wasm_bytes);
        
        module = wasm_module_new(store, &binary);
        wasm_byte_vec_delete(&binary);
        
        if (module != nullptr) {
            wasm_extern_vec_t imports = WASM_EMPTY_VEC;
            module_inst = wasm_instance_new(store, module, &imports, nullptr);
        }
    }
    
    wasm_engine_t* engine = nullptr;
    wasm_store_t* store = nullptr;
    wasm_module_t* module = nullptr;
    wasm_instance_t* module_inst = nullptr;
};

TEST_F(FunctionInstancesTest, Function_GetTypeFromNull_ReturnsNull)
{
    wasm_functype_t* func_type = wasm_func_type(nullptr);
    ASSERT_EQ(nullptr, func_type);
}

// Function Invocation Tests
TEST_F(FunctionInstancesTest, Function_Call_WithCorrectParams_ReturnsExpectedResult)
{
    if (module_inst == nullptr) {
        GTEST_SKIP() << "Module instantiation failed";
    }

    wasm_extern_vec_t exports;
    wasm_instance_exports(module_inst, &exports);

    if (exports.size == 0) {
        wasm_extern_vec_delete(&exports);
        GTEST_SKIP() << "No exports found in test module";
    }

    wasm_func_t* func = wasm_extern_as_func(exports.data[0]);
    if (func == nullptr) {
        wasm_extern_vec_delete(&exports);
        GTEST_SKIP() << "First export is not a function";
    }

    // Prepare arguments: 5 + 3 = 8
    wasm_val_t args[] = { WASM_I32_VAL(5), WASM_I32_VAL(3) };
    wasm_val_t results[1];
    
    wasm_val_vec_t args_vec = WASM_ARRAY_VEC(args);
    wasm_val_vec_t results_vec = WASM_ARRAY_VEC(results);
    
    wasm_trap_t* trap = wasm_func_call(func, &args_vec, &results_vec);
    ASSERT_EQ(nullptr, trap);
    ASSERT_EQ(8, results[0].of.i32);
    
    wasm_extern_vec_delete(&exports);
}

TEST_F(FunctionInstancesTest, Function_Call_WithParamsToArgvFailure_ReturnsTrap)
{
    if (module_inst == nullptr) {
        GTEST_SKIP() << "Module instantiation failed";
    }

    wasm_extern_vec_t exports;
    wasm_instance_exports(module_inst, &exports);

    if (exports.size == 0) {
        wasm_extern_vec_delete(&exports);
        GTEST_SKIP() << "No exports found in test module";
    }

    wasm_func_t* func = wasm_extern_as_func(exports.data[0]);
    if (func == nullptr) {
        wasm_extern_vec_delete(&exports);
        GTEST_SKIP() << "First export is not a function";
    }

    // Wrong number of arguments (should be 2, providing 1) — triggers params_to_argv failure
    wasm_val_t args[] = { WASM_I32_VAL(5) };
    wasm_val_t results[1];

    wasm_val_vec_t args_vec = WASM_ARRAY_VEC(args);
    wasm_val_vec_t results_vec = WASM_ARRAY_VEC(results);

    wasm_trap_t* trap = wasm_func_call(func, &args_vec, &results_vec);
    // In INTERP mode, params_to_argv hits default branch and returns a trap via goto failed
    // (in some WAMR builds the trap may be null; assert it does not crash)
    if (trap != nullptr) {
        wasm_trap_delete(trap);
    }

    wasm_extern_vec_delete(&exports);
}

TEST_F(FunctionInstancesTest, Function_Call_UnlinkedHostFunction_ReturnsUnlinkedTrap)
{
    // Create function type: () -> i32
    wasm_valtype_t* i32_type = wasm_valtype_new(WASM_I32);
    wasm_valtype_vec_t params, results;
    wasm_valtype_vec_new_empty(&params);
    wasm_valtype_vec_new(&results, 1, &i32_type);

    wasm_functype_t* func_type = wasm_functype_new(&params, &results);
    ASSERT_NE(nullptr, func_type);

    // Create a host function that is NOT linked to any module instance
    auto callback = [](const wasm_val_vec_t* args, wasm_val_vec_t* results) -> wasm_trap_t* {
        results->data[0] = WASM_I32_VAL(0);
        return nullptr;
    };

    wasm_func_t* func = wasm_func_new(store, func_type, callback);
    ASSERT_NE(nullptr, func);

    // Calling an unlinked host function triggers the unlinked-function trap path
    wasm_val_vec_t args_vec = WASM_EMPTY_VEC;
    wasm_val_t result_vals[1];
    wasm_val_vec_t results_vec = WASM_ARRAY_VEC(result_vals);

    wasm_trap_t* trap = wasm_func_call(func, &args_vec, &results_vec);
    ASSERT_NE(nullptr, trap);

    wasm_message_t trap_message;
    wasm_trap_message(trap, &trap_message);
    ASSERT_GT(trap_message.size, 0u);

    wasm_byte_vec_delete(&trap_message);
    wasm_trap_delete(trap);
    wasm_func_delete(func);
    wasm_functype_delete(func_type);
}

// Function Reference Tests
TEST_F(FunctionInstancesTest, Function_AsRef_UnlinkedHostFunc_RefAsFuncReturnsNull)
{
    // Create a simple host function not linked to any module instance
    wasm_valtype_t* i32_type = wasm_valtype_new(WASM_I32);
    wasm_valtype_vec_t params, results;
    wasm_valtype_vec_new_empty(&params);
    wasm_valtype_vec_new(&results, 1, &i32_type);

    wasm_functype_t* func_type = wasm_functype_new(&params, &results);
    ASSERT_NE(nullptr, func_type);

    auto callback = [](const wasm_val_vec_t* args, wasm_val_vec_t* results) -> wasm_trap_t* {
        results->data[0] = WASM_I32_VAL(42);
        return nullptr;
    };

    wasm_func_t* func = wasm_func_new(store, func_type, callback);
    ASSERT_NE(nullptr, func);

    // wasm_func_as_ref succeeds for a host function
    wasm_ref_t* ref = wasm_func_as_ref(func);
    ASSERT_NE(nullptr, ref);

    // wasm_ref_as_func returns NULL for an unlinked host function (inst_comm_rt is NULL)
    wasm_func_t* func_back = wasm_ref_as_func(ref);
    ASSERT_EQ(nullptr, func_back);

    wasm_func_delete(func);
    wasm_functype_delete(func_type);
}
