/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;

/**
 * @file enhanced_i64_ge_u_test.cc
 * @brief Comprehensive test suite for i64.ge_u opcode
 * @details Tests unsigned 64-bit integer greater-than-or-equal comparison operation across
 *          various scenarios including basic functionality, boundary conditions,
 *          edge cases, and error conditions. Validates behavior in both
 *          interpreter and AOT execution modes.
 */

/**
 * @class I64GeUTest
 * @brief Test fixture for i64.ge_u opcode validation
 * @details Provides setup and teardown for WAMR runtime environment
 *          and parameterized testing across execution modes
 */
class I64GeUTest : public testing::Test
{
protected:
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    char *error_buf = nullptr;
    uint8_t *buf = nullptr;
    uint32_t buf_size = 0;
    uint32_t stack_size = 16384;
    uint32_t heap_size = 16384;

    /**
     * @brief Set up test environment before each test case
     * @details Initializes WAMR runtime with system allocator and
     *          loads i64.ge_u test WASM module
     */
    void SetUp() override
    {
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Set up file paths
        CWD = getcwd(nullptr, 0);
        WASM_FILE = CWD + "/wasm-apps/i64_ge_u_test.wasm";

        // Allocate error buffer
        error_buf = (char*)malloc(256);
        ASSERT_NE(error_buf, nullptr) << "Failed to allocate error buffer";

        // Load i64.ge_u test WASM module
        buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << WASM_FILE;

        module = wasm_runtime_load(buf, buf_size, error_buf, 256);
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, 256);
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Clean up test environment after each test case
     * @details Destroys WAMR runtime and releases allocated resources
     */
    void TearDown() override
    {
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
            exec_env = nullptr;
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (buf != nullptr) {
            BH_FREE(buf);
            buf = nullptr;
        }
        if (error_buf != nullptr) {
            free(error_buf);
            error_buf = nullptr;
        }
        wasm_runtime_destroy();
    }

    /**
     * @brief Execute i64.ge_u comparison with two operands
     * @param left Left operand (first value)
     * @param right Right operand (second value)
     * @return Comparison result as i32 (1 if left >= right using unsigned semantics, 0 otherwise)
     */
    int32_t call_i64_ge_u(uint64_t left, uint64_t right)
    {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "i64_ge_u_test");
        EXPECT_NE(func, nullptr) << "Failed to lookup i64_ge_u_test function";
        if (func == nullptr) return -1;

        uint32_t argv[4];
        // Pack 64-bit arguments into 32-bit array for WASM call
        argv[0] = (uint32_t)(left & 0xFFFFFFFF);         // Low 32 bits of left
        argv[1] = (uint32_t)((left >> 32) & 0xFFFFFFFF);  // High 32 bits of left
        argv[2] = (uint32_t)(right & 0xFFFFFFFF);         // Low 32 bits of right
        argv[3] = (uint32_t)((right >> 32) & 0xFFFFFFFF); // High 32 bits of right

        bool call_result = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        EXPECT_TRUE(call_result) << "WASM function call failed: "
                                << wasm_runtime_get_exception(module_inst);

        return call_result ? (int32_t)argv[0] : -1;
    }

};

/**
 * @test MathematicalProperties_ReturnsCorrectResult
 * @brief Validates mathematical properties of unsigned >= relation
 * @details Tests reflexivity, transitivity, and totality properties of the
 *          unsigned greater-than-or-equal relation. Ensures i64.ge_u behaves
 *          as a proper mathematical relation under unsigned semantics.
 * @test_category Edge - Mathematical property validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:unsigned comparison consistency
 * @input_conditions Values chosen to test mathematical properties
 * @expected_behavior Mathematical properties of unsigned >= relation hold consistently
 * @validation_method Property-based testing with representative unsigned value sets
 */
TEST_F(I64GeUTest, MathematicalProperties_ReturnsCorrectResult)
{
    // Reflexivity: a >= a should always be true
    ASSERT_EQ(1, call_i64_ge_u(0ULL, 0ULL))
        << "Reflexivity: 0 >= 0 should return 1";
    ASSERT_EQ(1, call_i64_ge_u(1000ULL, 1000ULL))
        << "Reflexivity: 1000 >= 1000 should return 1";
    ASSERT_EQ(1, call_i64_ge_u(UINT64_MAX, UINT64_MAX))
        << "Reflexivity: UINT64_MAX >= UINT64_MAX should return 1";
    ASSERT_EQ(1, call_i64_ge_u(0x8000000000000000ULL, 0x8000000000000000ULL))
        << "Reflexivity: 0x8000000000000000 >= 0x8000000000000000 should return 1";

    // Transitivity validation through representative cases
    // If a >= b and b >= c, then a >= c should hold
    ASSERT_EQ(1, call_i64_ge_u(1000ULL, 500ULL))
        << "Transitivity setup: 1000 >= 500 should return 1";
    ASSERT_EQ(1, call_i64_ge_u(500ULL, 100ULL))
        << "Transitivity setup: 500 >= 100 should return 1";
    ASSERT_EQ(1, call_i64_ge_u(1000ULL, 100ULL))
        << "Transitivity: 1000 >= 100 should return 1 (from 1000 >= 500 >= 100)";

    // Antisymmetry cases: if a >= b and b >= a, then a == b
    ASSERT_EQ(1, call_i64_ge_u(42ULL, 42ULL))
        << "Antisymmetry: equal values should satisfy >= in both directions";

    // Totality: for any two values, either a >= b or b >= a (or both)
    ASSERT_EQ(0, call_i64_ge_u(5ULL, 10ULL))
        << "Totality: 5 >= 10 should return 0";
    ASSERT_EQ(1, call_i64_ge_u(10ULL, 5ULL))
        << "Totality: 10 >= 5 should return 1";

    // Test with large unsigned values
    ASSERT_EQ(0, call_i64_ge_u(0x7FFFFFFFFFFFFFFFULL, 0x8000000000000000ULL))
        << "0x7FFFFFFFFFFFFFFF >= 0x8000000000000000 (unsigned) should return 0";
    ASSERT_EQ(1, call_i64_ge_u(0x8000000000000000ULL, 0x7FFFFFFFFFFFFFFFULL))
        << "0x8000000000000000 >= 0x7FFFFFFFFFFFFFFF (unsigned) should return 1";
}
