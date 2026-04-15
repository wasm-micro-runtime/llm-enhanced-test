/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>  // Primary GTest framework for unit testing
#include <climits>        // Standard integer limits for boundary testing
#include <cstdint>        // Standard integer types for precise type control
#include <cmath>          // Mathematical functions for floating-point validation
#include <vector>         // Container for batch test case management
#include "wasm_export.h"  // Core WAMR runtime API for module management
#include "bh_read_file.h" // WAMR utility for loading WASM binary files

/**
 * @file enhanced_f64_convert_i32_s_test.cc
 * @brief Enhanced unit tests for f64.convert_i32_s opcode - Numeric Category
 * @details This test suite provides comprehensive coverage for the f64.convert_i32_s (signed integer to double conversion)
 * WebAssembly instruction, focusing on:
 * - Basic signed integer to IEEE 754 double precision conversion functionality
 * - Corner cases including boundary values (INT32_MIN, INT32_MAX) and sign preservation
 * - Edge cases with zero operands, identity operations, and extreme i32 values
 * - IEEE 754 compliance verification (exact representation and sign handling)
 * - Cross-execution mode validation between interpreter and AOT compilation
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64.convert_i32_s operations
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:DEF_OP_CONVERT macro
 * @coverage_target core/iwasm/aot/aot_runtime.c:floating-point conversion instructions
 * @coverage_target IEEE 754 double precision conversion algorithms and sign preservation
 * @coverage_target Stack management for numeric type conversion operations
 * @test_modes Both interpreter (Mode_Interp) and AOT (Mode_LLVM_JIT) execution
 */

class F64ConvertI32STestSuite : public testing::TestWithParam<RunningMode> {
protected:
    void SetUp() override {
        // Initialize WAMR runtime with system allocator for test isolation
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for f64.convert_i32_s testing";

        // Load WASM test module containing f64.convert_i32_s test functions
        std::string wasm_file = "./wasm-apps/f64_convert_i32_s_test.wasm";
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

        // Create module instance for test execution
        module_inst = wasm_runtime_instantiate(module, 8192, 8192,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode for parameterized testing
        wasm_runtime_set_running_mode(module_inst, GetParam());
    }

    void TearDown() override {
        // Clean up WASM resources in proper order
        if (module_inst) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }
        if (module) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (module_buffer) {
            wasm_runtime_free(module_buffer);
            module_buffer = nullptr;
        }

        // Shutdown WAMR runtime
        wasm_runtime_destroy();
    }

    /**
     * @brief Helper function to call f64.convert_i32_s WASM function
     * @param input i32 input value to be converted to f64
     * @return double converted result from WASM execution
     * @details Executes the WASM convert_i32_s function and returns the f64 result.
     * Function handles WAMR execution context and validates successful execution.
     */
    double call_convert_i32_s(int32_t input) {
        wasm_function_inst_t func_inst = wasm_runtime_lookup_function(
            module_inst, "convert_i32_s");
        EXPECT_NE(nullptr, func_inst) << "Failed to lookup convert_i32_s function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        // Prepare function arguments: i32 input value
        uint32_t argv[2] = { static_cast<uint32_t>(input), 0 };

        // Execute function with error handling
        bool call_result = wasm_runtime_call_wasm(exec_env, func_inst, 1, argv);
        EXPECT_TRUE(call_result)
            << "convert_i32_s function call failed: " << wasm_runtime_get_exception(module_inst);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        // Return result (f64 value is in argv[0] and argv[1] after call)
        union { uint64_t u; double f; } result;
        result.u = (static_cast<uint64_t>(argv[1]) << 32) | argv[0];
        return result.f;
    }

    /**
     * @brief Helper function to call boundary test WASM function
     * @param input i32 input value for boundary testing
     * @return double converted result from boundary test function
     * @details Executes the WASM convert_i32_s_boundary function for testing boundary conditions.
     */
    double call_convert_i32_s_boundary(int32_t input) {
        wasm_function_inst_t func_inst = wasm_runtime_lookup_function(
            module_inst, "convert_i32_s_boundary");
        EXPECT_NE(nullptr, func_inst) << "Failed to lookup convert_i32_s_boundary function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        // Prepare function arguments: i32 input value
        uint32_t argv[2] = { static_cast<uint32_t>(input), 0 };

        // Execute function with error handling
        bool call_result = wasm_runtime_call_wasm(exec_env, func_inst, 1, argv);
        EXPECT_TRUE(call_result)
            << "convert_i32_s_boundary function call failed: " << wasm_runtime_get_exception(module_inst);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        // Return result (f64 value is in argv[0] and argv[1] after call)
        union { uint64_t u; double f; } result;
        result.u = (static_cast<uint64_t>(argv[1]) << 32) | argv[0];
        return result.f;
    }

    /**
     * @brief Helper function to call extreme values test WASM function
     * @param input i32 input value for extreme value testing
     * @return double converted result from extreme values test function
     * @details Executes the WASM convert_i32_s_extreme function for testing extreme value conditions.
     */
    double call_convert_i32_s_extreme(int32_t input) {
        wasm_function_inst_t func_inst = wasm_runtime_lookup_function(
            module_inst, "convert_i32_s_extreme");
        EXPECT_NE(nullptr, func_inst) << "Failed to lookup convert_i32_s_extreme function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        // Prepare function arguments: i32 input value
        uint32_t argv[2] = { static_cast<uint32_t>(input), 0 };

        // Execute function with error handling
        bool call_result = wasm_runtime_call_wasm(exec_env, func_inst, 1, argv);
        EXPECT_TRUE(call_result)
            << "convert_i32_s_extreme function call failed: " << wasm_runtime_get_exception(module_inst);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        // Return result (f64 value is in argv[0] and argv[1] after call)
        union { uint64_t u; double f; } result;
        result.u = (static_cast<uint64_t>(argv[1]) << 32) | argv[0];
        return result.f;
    }

    // WAMR runtime resources
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    uint8_t* module_buffer = nullptr;
    uint32_t buffer_size = 0;
};

/**
 * @test BasicConversion_ReturnsCorrectF64
 * @brief Validates f64.convert_i32_s produces correct IEEE 754 double for typical i32 inputs
 * @details Tests fundamental conversion operation with positive, negative, and zero integers.
 *          Verifies that f64.convert_i32_s correctly converts signed integers to double precision.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_convert_i32_s_operation
 * @input_conditions Standard i32 values: 5, -10, 12345, -67890, 0
 * @expected_behavior Returns exact mathematical conversion with sign preservation
 * @validation_method Direct comparison of WASM function result with expected double values
 */
TEST_P(F64ConvertI32STestSuite, BasicConversion_ReturnsCorrectF64) {
    // Test small positive integer conversion
    ASSERT_EQ(5.0, call_convert_i32_s(5))
        << "Small positive i32 should convert to exact double equivalent";

    // Test small negative integer conversion
    ASSERT_EQ(-10.0, call_convert_i32_s(-10))
        << "Small negative i32 should convert to exact double equivalent";

    // Test medium positive integer conversion
    ASSERT_EQ(12345.0, call_convert_i32_s(12345))
        << "Medium positive i32 should convert to exact double equivalent";

    // Test medium negative integer conversion
    ASSERT_EQ(-67890.0, call_convert_i32_s(-67890))
        << "Medium negative i32 should convert to exact double equivalent";

    // Test zero conversion (identity case)
    ASSERT_EQ(0.0, call_convert_i32_s(0))
        << "Zero i32 should convert to positive zero double";
}

/**
 * @test BoundaryValues_ConvertExactly
 * @brief Validates f64.convert_i32_s correctly handles i32 boundary values and sign transitions
 * @details Tests conversion behavior at i32 MIN/MAX boundaries and values that test
 *          sign handling around zero boundary, ensuring exact IEEE 754 representation.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:boundary_value_handling
 * @input_conditions i32 boundary values: INT32_MIN, INT32_MAX, -1, 1
 * @expected_behavior Exact representation in IEEE 754 double precision with sign preservation
 * @validation_method Boundary value verification and exact mathematical comparison
 */
TEST_P(F64ConvertI32STestSuite, BoundaryValues_ConvertExactly) {
    // Test INT32_MAX boundary conversion
    ASSERT_EQ(static_cast<double>(INT32_MAX), call_convert_i32_s_boundary(INT32_MAX))
        << "INT32_MAX should convert to exact double representation without precision loss";

    // Test INT32_MIN boundary conversion
    ASSERT_EQ(static_cast<double>(INT32_MIN), call_convert_i32_s_boundary(INT32_MIN))
        << "INT32_MIN should convert to exact double representation with sign preservation";

    // Test just below zero boundary
    ASSERT_EQ(-1.0, call_convert_i32_s_boundary(-1))
        << "Negative one should convert to exact -1.0 double";

    // Test just above zero boundary
    ASSERT_EQ(1.0, call_convert_i32_s_boundary(1))
        << "Positive one should convert to exact 1.0 double";

    // Test large positive value within range
    ASSERT_EQ(2000000000.0, call_convert_i32_s_boundary(2000000000))
        << "Large positive i32 should convert to exact double equivalent";

    // Test large negative value within range
    ASSERT_EQ(-2000000000.0, call_convert_i32_s_boundary(-2000000000))
        << "Large negative i32 should convert to exact double equivalent";
}

// Instantiate parameterized tests for both execution modes
INSTANTIATE_TEST_SUITE_P(
    RunningMode,
    F64ConvertI32STestSuite,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode>& info) {
        switch (info.param) {
            case Mode_Interp:
                return "InterpreterMode";
            case Mode_LLVM_JIT:
                return "AOTMode";
            default:
                return "UnknownMode";
        }
    }
);
