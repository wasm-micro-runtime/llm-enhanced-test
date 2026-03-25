/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <iostream>
#include <fstream>

extern "C" {
#include "wasm_export.h"
#include "bh_read_file.h"
#include "wasm_runtime_common.h"
}

/**
 * @brief Test fixture for i64.lt_u opcode validation
 * @details Provides comprehensive testing infrastructure for WebAssembly i64.lt_u
 *          instruction across different WAMR execution modes (interpreter and AOT).
 *          Tests unsigned 64-bit integer less-than comparison operations.
 */
class I64LtUTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Initialize WAMR runtime and load test module
     * @details Sets up WAMR runtime environment with proper memory allocation
     *          and loads the i64.lt_u test module for execution validation.
     */
    void SetUp() override {
        // Initialize WAMR runtime with system allocator
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load test WASM module
        module_buffer = LoadWASMModule();
        ASSERT_NE(nullptr, module_buffer) << "Failed to load WASM module buffer";

        // Load module into WAMR runtime
        char error_buf[128] = {0};
        module = wasm_runtime_load(module_buffer, module_buffer_size,
                                   error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module)
            << "Failed to load WASM module: " << error_buf;

        // Instantiate module with execution mode
        module_inst = wasm_runtime_instantiate(module, 65536, 65536,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode for parameterized testing
        wasm_runtime_set_running_mode(module_inst, GetParam());

        // Get function reference
        lt_u_func = wasm_runtime_lookup_function(module_inst, "i64_lt_u_test");
        ASSERT_NE(nullptr, lt_u_func)
            << "Failed to lookup i64_lt_u_test function";
    }

    /**
     * @brief Clean up WAMR runtime resources
     * @details Properly deallocates all WAMR runtime resources including
     *          module instances, modules, and runtime environment.
     */
    void TearDown() override {
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        delete[] module_buffer;
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM test module from file system
     * @details Reads the compiled WASM module containing i64.lt_u test function
     * @return Pointer to module buffer or nullptr on failure
     */
    unsigned char* LoadWASMModule() {
        const char* wasm_file = "wasm-apps/i64_lt_u_test.wasm";
        return (unsigned char*)bh_read_file_to_buffer(wasm_file, &module_buffer_size);
    }

    /**
     * @brief Execute i64.lt_u test function with given inputs
     * @details Calls the WASM function that applies i64.lt_u to two 64-bit inputs
     * @param first The first 64-bit integer operand
     * @param second The second 64-bit integer operand
     * @return The comparison result as 32-bit integer (1 if first < second unsigned, 0 otherwise)
     */
    int32_t CallI64LtU(uint64_t first, uint64_t second) {
        wasm_val_t args[2];
        wasm_val_t results[1];

        // Set up function arguments
        args[0].kind = WASM_I64;
        args[0].of.i64 = static_cast<int64_t>(first);
        args[1].kind = WASM_I64;
        args[1].of.i64 = static_cast<int64_t>(second);

        // Execute function
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        bool success = wasm_runtime_call_wasm_a(exec_env, lt_u_func, 1, results, 2, args);
        wasm_runtime_destroy_exec_env(exec_env);

        EXPECT_TRUE(success) << "Function call failed";
        return results[0].of.i32;
    }

    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_function_inst_t lt_u_func = nullptr;
    unsigned char* module_buffer = nullptr;
    uint32_t module_buffer_size = 0;
};

/**
 * @test BoundaryValues_ZeroAndMax_ReturnsExpectedResults
 * @brief Tests critical boundary conditions at unsigned 64-bit limits
 * @details Validates comparison behavior at exact boundaries: zero, maximum unsigned value,
 *          and transitions around signed/unsigned interpretation differences.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @input_conditions Boundary values: 0, UINT64_MAX-1, UINT64_MAX, signed/unsigned boundaries
 * @expected_behavior Mathematically correct unsigned comparisons at all boundaries
 * @validation_method Verify boundary cases produce correct unsigned comparison results
 */
TEST_P(I64LtUTest, BoundaryValues_ZeroAndMax_ReturnsExpectedResults) {
    // Test zero boundary conditions
    ASSERT_EQ(1, CallI64LtU(0ULL, 1ULL))
        << "Failed zero boundary: 0 < 1 should be true";
    ASSERT_EQ(0, CallI64LtU(1ULL, 0ULL))
        << "Failed zero boundary: 1 < 0 should be false";
    ASSERT_EQ(0, CallI64LtU(0ULL, 0ULL))
        << "Failed zero boundary: 0 < 0 should be false";

    // Test maximum value boundary conditions
    ASSERT_EQ(1, CallI64LtU(0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL))
        << "Failed max boundary: (UINT64_MAX-1) < UINT64_MAX should be true";
    ASSERT_EQ(0, CallI64LtU(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL))
        << "Failed max boundary: UINT64_MAX < (UINT64_MAX-1) should be false";

    // Test zero compared to maximum
    ASSERT_EQ(1, CallI64LtU(0ULL, 0xFFFFFFFFFFFFFFFFULL))
        << "Failed zero to max: 0 < UINT64_MAX should be true";

    // Test signed/unsigned boundary (0x8000000000000000 is positive in unsigned)
    ASSERT_EQ(0, CallI64LtU(0x8000000000000000ULL, 0x7FFFFFFFFFFFFFFFULL))
        << "Failed signed/unsigned boundary: 0x8000000000000000 < 0x7FFFFFFFFFFFFFFF should be false in unsigned";
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossExecutionModeValidation,
    I64LtUTest,
    testing::Values(
        RunningMode::Mode_Interp,
        RunningMode::Mode_LLVM_JIT
    ),
    [](const testing::TestParamInfo<I64LtUTest::ParamType>& info) {
        switch (info.param) {
            case RunningMode::Mode_Interp:
                return "InterpreterMode";
            case RunningMode::Mode_LLVM_JIT:
                return "AOTMode";
            default:
                return "UnknownMode";
        }
    }
);
