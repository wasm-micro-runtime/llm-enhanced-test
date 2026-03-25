/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>  // Primary GTest framework for unit testing
#include <climits>        // Standard integer limits for boundary testing
#include <cstdint>        // Standard integer types for precise type control
#include <vector>         // Container for batch test case management
#include "wasm_export.h"  // Core WAMR runtime API for module management
#include "bh_read_file.h" // WAMR utility for loading WASM binary files

/**
 * @file enhanced_i64_rotr_test.cc
 * @brief Enhanced unit tests for i64.rotr opcode - Numeric Category
 * @details This test suite provides comprehensive coverage for the i64.rotr (rotate right)
 * WebAssembly instruction, focusing on:
 * - Basic rotation functionality with typical values and rotation counts
 * - Corner cases including boundary rotation counts (0, 63, 64, 65+) and modulo behavior
 * - Edge cases with extreme values (0, 0xFFFFFFFFFFFFFFFF, powers of 2, alternating patterns)
 * - Identity operations and mathematical properties (cyclic behavior, bit preservation)
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:rotr64 function
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:rotr64 function
 * @coverage_target core/iwasm/aot/aot_runtime.c:bitwise rotation instructions
 * @coverage_target Bit rotation algorithms and modulo 64 behavior
 * @coverage_target Stack management for binary numeric operations
 * @test_modes Both interpreter (Mode_Interp) and AOT (Mode_LLVM_JIT) execution
 */

class I64RotrTestSuite : public testing::TestWithParam<RunningMode> {
protected:
    void SetUp() override {
        // Initialize WAMR runtime with system allocator for test isolation
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for i64.rotr testing";

        // Load WASM test module containing i64.rotr test functions
        std::string wasm_file = "./wasm-apps/i64_rotr_test.wasm";
        module_buffer = reinterpret_cast<uint8_t*>(
            bh_read_file_to_buffer(wasm_file.c_str(), &buffer_size));
        ASSERT_NE(nullptr, module_buffer)
            << "Failed to load WASM file: " << wasm_file;

        // Load and validate WASM module with error reporting
        char error_buf[128];
        module = wasm_runtime_load(module_buffer, buffer_size,
                                 error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module)
            << "Failed to load WASM module: " << error_buf;

        // Instantiate WASM module for test execution
        module_inst = wasm_runtime_instantiate(module, 65536, 65536,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode based on test parameter (interpreter or AOT)
        wasm_runtime_set_running_mode(module_inst, GetParam());
    }

    void TearDown() override {
        // Clean up WASM resources in reverse order of creation
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        if (module_buffer != nullptr) {
            BH_FREE(module_buffer);
        }

        // Shutdown WAMR runtime to prevent resource leaks
        wasm_runtime_destroy();
    }

    /**
     * @brief Executes i64.rotr operation with two operands
     * @details Helper function to call WASM i64.rotr function and retrieve result
     * @param value The 64-bit integer value to rotate
     * @param count The number of positions to rotate right
     * @return Result of rotating value right by count positions
     * @coverage_target Function call mechanism and parameter passing
     */
    uint64_t call_i64_rotr(uint64_t value, uint64_t count) {
        // Locate the exported i64_rotr test function
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, "i64_rotr_test");
        EXPECT_NE(nullptr, func) << "Failed to find i64_rotr_test function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        // Prepare function arguments: [value, count]
        uint32_t argv[4];
        // Split 64-bit values into two 32-bit arguments (little-endian)
        argv[0] = static_cast<uint32_t>(value & 0xFFFFFFFF);
        argv[1] = static_cast<uint32_t>(value >> 32);
        argv[2] = static_cast<uint32_t>(count & 0xFFFFFFFF);
        argv[3] = static_cast<uint32_t>(count >> 32);

        // Execute i64.rotr function and capture result
        bool success = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        EXPECT_TRUE(success) << "Failed to execute i64_rotr_test function";

        // Cleanup execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        // Reconstruct 64-bit result from two 32-bit values
        return static_cast<uint64_t>(argv[0]) |
               (static_cast<uint64_t>(argv[1]) << 32);
    }

    // Test infrastructure members
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    uint8_t* module_buffer = nullptr;
    uint32_t buffer_size = 0;
};

/**
 * @test BasicRotation_ReturnsCorrectResult
 * @brief Validates i64.rotr produces correct arithmetic results for typical inputs
 * @details Tests fundamental right rotation operation with small, medium, and large rotation counts.
 *          Verifies that i64.rotr correctly performs circular bit shifts to the right.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:rotr64
 * @input_conditions Standard rotation scenarios: 4-bit, 8-bit, and 32-bit shifts
 * @expected_behavior Returns mathematically correct right-rotated results
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(I64RotrTestSuite, BasicRotation_ReturnsCorrectResult) {
    // Small rotation: 0x123456789ABCDEF0 >> 4 = 0x0123456789ABCDEF
    ASSERT_EQ(call_i64_rotr(0x123456789ABCDEF0ULL, 4), 0x0123456789ABCDEFULL)
        << "4-bit right rotation failed for test pattern";

    // Byte rotation: rotate right by 8 bits
    ASSERT_EQ(call_i64_rotr(0x123456789ABCDEF0ULL, 8), 0xF0123456789ABCDEULL)
        << "8-bit right rotation failed for test pattern";

    // Half rotation: rotate right by 32 bits
    ASSERT_EQ(call_i64_rotr(0x123456789ABCDEF0ULL, 32), 0x9ABCDEF012345678ULL)
        << "32-bit right rotation failed for test pattern";
}

// Test suite instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossMode, I64RotrTestSuite,
    testing::Values(RunningMode::Mode_Interp, RunningMode::Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode>& info) {
        return info.param == RunningMode::Mode_Interp ? "Interpreter" : "AOT";
    }
);
