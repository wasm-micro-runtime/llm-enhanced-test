/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static int
app_argc;
static char **app_argv;

/**
 * @brief Test fixture for table.set opcode validation
 *
 * This test suite validates the comprehensive functionality of the table.set
 * WebAssembly opcode across different execution modes (interpreter and AOT).
 * Tests cover basic functionality, boundary conditions, edge cases, and error scenarios.
 */
class TableSetTest : public testing::Test
{
protected:
    /**
     * @brief Set up test environment and initialize WAMR runtime
     *
     * Initializes WAMR runtime with proper configuration for both interpreter
     * and AOT execution modes, sets up memory allocation, and prepares test
     * resources for table.set opcode testing.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_EQ(wasm_runtime_full_init(&init_args), true);

        cleanup_wasmfile = false;
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
    }

    /**
     * @brief Clean up test environment and destroy WAMR runtime
     *
     * Properly deallocates all WAMR resources, destroys module instances,
     * and ensures clean shutdown of the runtime environment to prevent
     * resource leaks between test cases.
     */
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
        // Cleanup handled by WAMR runtime destruction
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM module from file and create module instance
     *
     * @param filename Name of the WASM file to load from wasm-apps directory
     * @return bool True if module loaded successfully, false otherwise
     */
    bool load_wasm_module(const char *filename)
    {
        wasm_file_buf = (char *)bh_read_file_to_buffer(filename, &wasm_file_size);

        if (wasm_file_buf == nullptr) {
            return false;
        }

        module = wasm_runtime_load((uint8_t *)wasm_file_buf, wasm_file_size,
                                 error_buf, sizeof(error_buf));
        if (module == nullptr) {
            return false;
        }

        module_inst = wasm_runtime_instantiate(
            module, 65536, 65536, error_buf, sizeof(error_buf));
        if (module_inst == nullptr) {
            return false;
        }

        exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        return exec_env != nullptr;
    }

    /**
     * @brief Call WASM function to set funcref in table
     *
     * @param table_index Table index to operate on
     * @param elem_index Element index within the table
     * @param func_index Function index to store as reference
     * @return bool True if operation succeeds, false if trap occurs
     */
    bool call_table_set_funcref(uint32_t table_index, uint32_t elem_index, uint32_t func_index)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "table_set_funcref");
        if (func == nullptr) {
            return false;
        }

        uint32_t argv[3] = { table_index, elem_index, func_index };
        return wasm_runtime_call_wasm(exec_env, func, 3, argv);
    }

    /**
     * @brief Call WASM function to set externref in table
     *
     * @param table_index Table index to operate on
     * @param elem_index Element index within the table
     * @param ref_value External reference value to store
     * @return bool True if operation succeeds, false if trap occurs
     */
    bool call_table_set_externref(uint32_t table_index, uint32_t elem_index, uint32_t ref_value)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "table_set_externref");
        if (func == nullptr) {
            return false;
        }

        uint32_t argv[3] = { table_index, elem_index, ref_value };
        return wasm_runtime_call_wasm(exec_env, func, 3, argv);
    }

    /**
     * @brief Call WASM function to get funcref from table
     *
     * @param table_index Table index to operate on
     * @param elem_index Element index within the table
     * @return uint32_t Function index stored at position, or invalid value if null
     */
    uint32_t call_table_get_funcref(uint32_t table_index, uint32_t elem_index)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "table_get_funcref");
        if (func == nullptr) {
            return UINT32_MAX;
        }

        uint32_t argv[2] = { table_index, elem_index };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        return ret ? argv[0] : UINT32_MAX;
    }

    /**
     * @brief Call WASM function to get externref from table
     *
     * @param table_index Table index to operate on
     * @param elem_index Element index within the table
     * @return uint32_t External reference value stored at position
     */
    uint32_t call_table_get_externref(uint32_t table_index, uint32_t elem_index)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "table_get_externref");
        if (func == nullptr) {
            return UINT32_MAX;
        }

        uint32_t argv[2] = { table_index, elem_index };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        return ret ? argv[0] : UINT32_MAX;
    }

    /**
     * @brief Call WASM function to set null funcref in table
     *
     * @param table_index Table index to operate on
     * @param elem_index Element index within the table
     * @return bool True if null reference set successfully
     */
    bool call_table_set_null_funcref(uint32_t table_index, uint32_t elem_index)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "table_set_null_funcref");
        if (func == nullptr) {
            return false;
        }

        uint32_t argv[2] = { table_index, elem_index };
        return wasm_runtime_call_wasm(exec_env, func, 2, argv);
    }

    /**
     * @brief Call WASM function to set null externref in table
     *
     * @param table_index Table index to operate on
     * @param elem_index Element index within the table
     * @return bool True if null reference set successfully
     */
    bool call_table_set_null_externref(uint32_t table_index, uint32_t elem_index)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "table_set_null_externref");
        if (func == nullptr) {
            return false;
        }

        uint32_t argv[2] = { table_index, elem_index };
        return wasm_runtime_call_wasm(exec_env, func, 2, argv);
    }

    RuntimeInitArgs init_args;
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;
    char error_buf[128];
    char *wasm_file_buf;
    uint32_t wasm_file_size;
    bool cleanup_wasmfile;
};

/**
 * @test BasicTableSet_ExternalReference_StoresCorrectly
 * @brief Validates table.set stores external references correctly in externref table
 * @details Tests basic table.set operation with external reference values.
 *          Verifies that externref values are properly stored and maintained
 *          through storage and retrieval operations.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:table_set_operation
 * @input_conditions Valid externref values (42, 100, 255) at table positions (0,1,2)
 * @expected_behavior External references stored successfully and retrievable
 * @validation_method Round-trip validation comparing set and retrieved values
 */
TEST_F(TableSetTest, BasicTableSet_ExternalReference_StoresCorrectly)
{
    ASSERT_TRUE(load_wasm_module("wasm-apps/table_set_test.wasm"))
        << "Failed to load table.set test module";

    // Test setting external references at different table positions
    ASSERT_TRUE(call_table_set_externref(1, 0, 42))
        << "Failed to set external reference at externref table[0]";
    ASSERT_TRUE(call_table_set_externref(1, 1, 100))
        << "Failed to set external reference at externref table[1]";
    ASSERT_TRUE(call_table_set_externref(1, 2, 255))
        << "Failed to set external reference at externref table[2]";

    // Validate round-trip storage and retrieval
    ASSERT_EQ(42U, call_table_get_externref(1, 0))
        << "External reference at externref table[0] does not match expected value";
    ASSERT_EQ(100U, call_table_get_externref(1, 1))
        << "External reference at externref table[1] does not match expected value";
    ASSERT_EQ(255U, call_table_get_externref(1, 2))
        << "External reference at externref table[2] does not match expected value";
}

int
main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
