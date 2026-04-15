/*
 * Copyright (C) 2024 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include <unistd.h>
#include <fstream>
#include "wasm_runtime_common.h"
#include "wasm_runtime.h"
#include "wasm_interp.h"
#include "wasm_loader.h"

static std::string CWD;
static std::string WASM_FILE;
static constexpr uint32_t STACK_SIZE = 8092;
static constexpr uint32_t HEAP_SIZE = 8092;

static int
test_import_add_impl(wasm_exec_env_t exec_env, int32_t a, int32_t b)
{
    return a + b;
}

static int
test_import_mul_impl(wasm_exec_env_t exec_env, int32_t a, int32_t b)
{
    return a * b;
}

static int
malloc_impl(wasm_exec_env_t exec_env, int32_t size)
{
    return static_cast<int>(wasm_runtime_module_malloc(
        wasm_runtime_get_module_inst(exec_env), size, nullptr));
}

static void
free_impl(wasm_exec_env_t exec_env, int32_t ptr)
{
    wasm_runtime_module_free(
        wasm_runtime_get_module_inst(exec_env), ptr);
}

static int
native_func_impl(wasm_exec_env_t exec_env, int32_t a)
{
    return a * 2;
}

static NativeSymbol native_symbols[] = {
    { "test_import_add", (void *)test_import_add_impl, "(ii)i", nullptr },
    { "test_import_mul", (void *)test_import_mul_impl, "(ii)i", nullptr },
    { "malloc", (void *)malloc_impl, "(i)i", nullptr },
    { "free", (void *)free_impl, "(i)", nullptr },
    { "native_func", (void *)native_func_impl, "(i)i", nullptr }
};

/**
 * Test fixture for Step 3: Function Invocation and Stack Operations
 * Targets 6 functions: call_indirect, wasm_call_indirect, wasm_interp_call_func_import,
 * copy_stack_values, execute_malloc_function, execute_free_function
 */
class FunctionInvocationTest : public testing::Test
{
protected:
    void SetUp() override
    {
        char *current_dir = getcwd(nullptr, 0);
        CWD = std::string(current_dir);
        free(current_dir);
        WASM_FILE = CWD + "/function_invocation_test.wasm";

        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args));

        // Register native symbols for import testing
        ASSERT_TRUE(wasm_runtime_register_natives("env", native_symbols,
                                                  sizeof(native_symbols) / sizeof(NativeSymbol)));

        load_wasm_file();
        instantiate_module();
    }

    void TearDown() override
    {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        delete[] wasm_file_buf;
        wasm_runtime_destroy();
    }

    void load_wasm_file()
    {
        std::ifstream wasm_file(WASM_FILE, std::ios::binary);
        ASSERT_TRUE(wasm_file.is_open()) << "Failed to open WASM file: " << WASM_FILE;
        
        std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(wasm_file), {});
        wasm_file_size = buffer.size();
        wasm_file_buf = new unsigned char[wasm_file_size];
        std::copy(buffer.begin(), buffer.end(), wasm_file_buf);

        module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf,
                                   sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Load module failed: " << error_buf;
    }

    void instantiate_module()
    {
        module_inst = wasm_runtime_instantiate(
            module, STACK_SIZE, HEAP_SIZE, error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Instantiate module failed: " << error_buf;

        exec_env = wasm_runtime_create_exec_env(module_inst, STACK_SIZE);
        ASSERT_NE(exec_env, nullptr);
    }

    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    unsigned char *wasm_file_buf = nullptr;
    uint32_t wasm_file_size = 0;
    char error_buf[128] = { 0 };
};

/**
 * Test Function 1: call_indirect() - Invalid index
 * Target: core/iwasm/interpreter/wasm_runtime.c:call_indirect()
 * Expected Coverage: ~15 lines (error path)
 */
TEST_F(FunctionInvocationTest, CallIndirect_InvalidIndex_FailsGracefully)
{
    wasm_function_inst_t func = wasm_runtime_lookup_function(
        module_inst, "test_call_indirect_invalid_index");
    ASSERT_NE(func, nullptr);

    uint32_t wasm_argv[1];
    wasm_argv[0] = 42; // input value

    bool success = wasm_runtime_call_wasm(exec_env, func, 1, wasm_argv);
    ASSERT_FALSE(success);
}

/**
 * Test Function 2: wasm_call_indirect() - Type mismatch error handling
 * Target: core/iwasm/interpreter/wasm_runtime.c:wasm_call_indirect()
 * Expected Coverage: ~13 lines (error path)
 */
TEST_F(FunctionInvocationTest, WasmCallIndirect_TypeMismatch_ReturnsFailure)
{
    wasm_function_inst_t func = wasm_runtime_lookup_function(
        module_inst, "test_call_indirect_type_mismatch");
    ASSERT_NE(func, nullptr);

    uint32_t wasm_argv[1];
    wasm_argv[0] = 100; // input value

    bool success = wasm_runtime_call_wasm(exec_env, func, 1, wasm_argv);
    ASSERT_FALSE(success);
}

/**
 * Test Function 5: execute_malloc_function() - Failure path
 * Target: core/iwasm/interpreter/wasm_runtime.c:execute_malloc_function()
 * Expected Coverage: ~20 lines (failure path)
 */
TEST_F(FunctionInvocationTest, ExecuteMalloc_Failure_HandlesLargeAllocation)
{
    wasm_function_inst_t func = wasm_runtime_lookup_function(
        module_inst, "test_malloc_operation");
    ASSERT_NE(func, nullptr);

    uint32_t wasm_argv[1];
    wasm_argv[0] = 0x10000000; // try to allocate large amount (256MB)

    bool success = wasm_runtime_call_wasm(exec_env, func, 1, wasm_argv);
    // Large allocation should either fail at call level or return 0 (null pointer)
    if (success) {
        ASSERT_EQ(wasm_argv[0], static_cast<uint32_t>(0));
    }
    else {
        // Verify that an exception was set on failure
        const char *exception = wasm_runtime_get_exception(module_inst);
        ASSERT_NE(exception, nullptr);
    }
}

/**
 * Test malloc/free cycle to exercise both functions together
 */
TEST_F(FunctionInvocationTest, MallocFreeCycle_Complete_WorksCorrectly)
{
    wasm_function_inst_t func = wasm_runtime_lookup_function(
        module_inst, "test_malloc_free_cycle");
    ASSERT_NE(func, nullptr);

    uint32_t wasm_argv[1];
    wasm_argv[0] = 256; // allocation size

    bool success = wasm_runtime_call_wasm(exec_env, func, 1, wasm_argv);
    ASSERT_TRUE(success);

    // Should return the value that was stored (42)
    ASSERT_EQ(wasm_argv[0], 42);
}

/**
 * Test complex indirect call scenarios
 */
TEST_F(FunctionInvocationTest, ComplexIndirectCalls_MultipleSelectors_HandlesCorrectly)
{
    wasm_function_inst_t func = wasm_runtime_lookup_function(
        module_inst, "test_complex_indirect_calls");
    ASSERT_NE(func, nullptr);

    // Test selector 0 (add_func)
    uint32_t wasm_argv[2];
    wasm_argv[0] = 0;  // selector
    wasm_argv[1] = 30; // value

    bool success = wasm_runtime_call_wasm(exec_env, func, 2, wasm_argv);
    ASSERT_TRUE(success);
    ASSERT_EQ(wasm_argv[0], 40); // 30 + 10 = 40

    // Test selector 1 (mul_func)
    wasm_argv[0] = 1;  // selector
    wasm_argv[1] = 15; // value

    success = wasm_runtime_call_wasm(exec_env, func, 2, wasm_argv);
    ASSERT_TRUE(success);
    ASSERT_EQ(wasm_argv[0], 30); // 15 * 2 = 30

    // Test selector 2 (identity_func)
    wasm_argv[0] = 2;  // selector
    wasm_argv[1] = 99; // value

    success = wasm_runtime_call_wasm(exec_env, func, 2, wasm_argv);
    ASSERT_TRUE(success);
    ASSERT_EQ(wasm_argv[0], 99); // identity returns same value
}
