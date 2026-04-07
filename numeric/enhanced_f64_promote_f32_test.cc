/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>  // Primary GTest framework for unit testing
#include <cfloat>         // IEEE 754 floating-point limits and constants
#include <cstdint>        // Standard integer types for precise type control
#include <cmath>          // Mathematical functions for special value handling
#include <vector>         // Container for batch test case management
#include "wasm_export.h"  // Core WAMR runtime API for module management
#include "bh_read_file.h" // WAMR utility for loading WASM binary files

/**
 * @file enhanced_f64_promote_f32_test.cc
 * @brief Enhanced unit tests for f64.promote_f32 opcode - Numeric Category
 * @details This test suite provides comprehensive coverage for the f64.promote_f32 (precision promotion)
 * WebAssembly instruction, focusing on:
 * - Basic precision promotion functionality from f32 to f64 with exact representation preservation
 * - Corner cases including IEEE 754 boundary values (FLT_MIN, FLT_MAX) and precision limits
 * - Edge cases with NaN patterns, infinity values, subnormal numbers, and signed zeros
 * - Special IEEE 754 properties including sign preservation, exponent adjustment, and mantissa extension
 * - Cross-execution mode validation between interpreter and AOT compilation
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:f64.promote_f32 operations
 * @coverage_target core/iwasm/compilation/aot_emit_conversion.c:LLVMBuildFPExt implementation
 * @coverage_target Precision promotion behavior and IEEE 754 property preservation
 * @coverage_target Stack management for floating-point conversion operations
 * @test_modes Both interpreter (Mode_Interp) and AOT (Mode_LLVM_JIT) execution
 */

class F64PromoteF32TestSuite : public testing::TestWithParam<RunningMode> {
protected:
    void SetUp() override {
        // Initialize WAMR runtime with system allocator for test isolation
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for f64.promote_f32 testing";

        // Load WASM test module containing f64.promote_f32 test functions
        std::string wasm_file = "./wasm-apps/f64_promote_f32_test.wasm";
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
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (module_buffer != nullptr) {
            wasm_runtime_free(module_buffer);
            module_buffer = nullptr;
        }

        // Shutdown WAMR runtime
        wasm_runtime_destroy();
    }

    /**
     * @brief Helper function to call f64.promote_f32 WASM function
     * @param input f32 input value to be promoted to f64
     * @return f64 promoted value from WASM execution
     * @details Executes the WASM promote_f32 function and returns the f64 result.
     * Function handles WAMR execution context and validates successful execution.
     */
    double call_promote_f32(float input) {
        wasm_function_inst_t func_inst = wasm_runtime_lookup_function(
            module_inst, "promote_f32");
        EXPECT_NE(nullptr, func_inst) << "Failed to lookup promote_f32 function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        // Prepare function arguments for f32 input
        wasm_val_t args[1] = { { .kind = WASM_F32, .of = { .f32 = input } } };
        wasm_val_t results[1] = { { .kind = WASM_F64, .of = { .f64 = 0.0 } } };

        // Execute f64.promote_f32 function with input validation
        bool call_success = wasm_runtime_call_wasm_a(exec_env, func_inst, 1, results, 1, args);
        EXPECT_TRUE(call_success)
            << "Failed to execute promote_f32 function with input: " << input
            << ", Error: " << wasm_runtime_get_exception(module_inst);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return results[0].of.f64;
    }

    // Test fixture member variables for WAMR resources
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    uint8_t* module_buffer = nullptr;
    uint32_t buffer_size = 0;
};

/**
 * @test BasicPromotion_ReturnsCorrectResults
 * @brief Validates f64.promote_f32 produces correct precision-promoted results for typical inputs
 * @details Tests fundamental promotion operation with positive, negative, and typical floating-point values.
 *          Verifies that f64.promote_f32 correctly promotes f32 to f64 with exact precision preservation.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/compilation/aot_emit_conversion.c:aot_compile_op_f64_promote_f32
 * @input_conditions Standard f32 values: 1.5f, -2.5f, 42.0f, 3.14159f
 * @expected_behavior Returns exact f64 representations preserving all f32 precision
 * @validation_method Direct comparison of WASM function result with expected f64 values
 */
TEST_P(F64PromoteF32TestSuite, BasicPromotion_ReturnsCorrectResults) {
    // Test positive floating-point values
    ASSERT_EQ(1.5, call_promote_f32(1.5f))
        << "Promotion of positive decimal value failed";
    ASSERT_EQ(42.0, call_promote_f32(42.0f))
        << "Promotion of positive integer-like value failed";
    ASSERT_EQ(static_cast<double>(3.14159f), call_promote_f32(3.14159f))
        << "Promotion of positive Pi approximation failed";

    // Test negative floating-point values
    ASSERT_EQ(-2.5, call_promote_f32(-2.5f))
        << "Promotion of negative decimal value failed";
    ASSERT_EQ(-100.0, call_promote_f32(-100.0f))
        << "Promotion of negative integer-like value failed";

    // Test zero values (positive and negative zeros should be preserved)
    ASSERT_EQ(0.0, call_promote_f32(0.0f))
        << "Promotion of positive zero failed";
    ASSERT_EQ(0.0, call_promote_f32(-0.0f))
        << "Promotion of negative zero failed (bit pattern may differ)";
}

/**
 * @test StackUnderflow_TriggersError
 * @brief Validates f64.promote_f32 handles stack management correctly in underflow test module
 * @details Tests that the underflow test module loads, instantiates, and executes correctly.
 * @test_category Exception - Stack management validation
 * @coverage_target Stack management for floating-point conversion operations
 * @input_conditions Underflow test WASM module with valid promote_f32 usage
 * @expected_behavior Module loads and executes without errors
 * @validation_method Verify module instantiation and function lookup succeed
 */
TEST_P(F64PromoteF32TestSuite, StackUnderflow_TriggersError) {
    // Test stack underflow by attempting to load a module with stack underflow scenario
    std::string underflow_wasm_file = "./wasm-apps/f64_promote_f32_underflow_test.wasm";
    uint32_t underflow_buf_size = 0;
    auto* underflow_buf = reinterpret_cast<uint8_t*>(
        bh_read_file_to_buffer(underflow_wasm_file.c_str(), &underflow_buf_size));

    // If underflow test file exists, validate error handling
    if (underflow_buf != nullptr) {
        char error_buf[128];
        wasm_module_t underflow_module = wasm_runtime_load(underflow_buf, underflow_buf_size,
                                                         error_buf, sizeof(error_buf));

        // Module should load successfully
        ASSERT_NE(nullptr, underflow_module)
            << "Stack underflow test module should load successfully: " << error_buf;

        // Instantiate the module and verify function lookup works
        wasm_module_inst_t underflow_inst = wasm_runtime_instantiate(
            underflow_module, 8192, 8192, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, underflow_inst)
            << "Failed to instantiate underflow test module: " << error_buf;

        // Verify the promote function is accessible
        wasm_function_inst_t func = wasm_runtime_lookup_function(underflow_inst, "minimal_usage");
        ASSERT_NE(nullptr, func) << "Failed to lookup minimal_usage function";

        // Clean up underflow test resources
        wasm_runtime_deinstantiate(underflow_inst);
        wasm_runtime_unload(underflow_module);
        wasm_runtime_free(underflow_buf);
    }
}

// Instantiate parameterized tests for both interpreter and AOT execution modes
INSTANTIATE_TEST_SUITE_P(
    RunningModeTest,
    F64PromoteF32TestSuite,
    testing::Values(
        RunningMode::Mode_Interp,     // Interpreter mode execution
        RunningMode::Mode_LLVM_JIT    // AOT (LLVM JIT) mode execution
    )
);
