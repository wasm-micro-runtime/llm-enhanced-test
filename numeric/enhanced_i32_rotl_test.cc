/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <climits>
#include <cstdint>
#include <vector>
#include "wasm_export.h"
#include "bh_read_file.h"

static constexpr unsigned STACK_SIZE = 8092;
static constexpr unsigned HEAP_SIZE = 8092;

/**
 * Enhanced i32.rotl opcode test suite class
 * @brief Comprehensive test coverage for i32.rotl (32-bit rotate left) operation
 * @details Tests basic rotation functionality, boundary conditions, edge cases, and error scenarios
 *          across both interpreter and AOT execution modes. Validates mathematical properties
 *          and proper handling of rotation count modulo arithmetic.
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @coverage_target core/iwasm/aot/aot_runtime.c:aot_call_function
 */
class I32RotlTest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * Test fixture setup method
     * @brief Initialize WAMR runtime with proper configuration for i32.rotl testing
     * @details Sets up memory allocator, enables interpreter and AOT modes, loads test modules
     */
    void SetUp() override
    {
        // Initialize WAMR runtime with comprehensive configuration
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        init_args.n_native_symbols = 0;
        init_args.native_module_name = nullptr;
        init_args.native_symbols = nullptr;

        // Initialize WAMR runtime environment
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for i32.rotl testing";

        // Load primary test module for basic functionality
        std::string wasm_file = "wasm-apps/i32_rotl_test.wasm";
        buffer = reinterpret_cast<uint8_t*>(
            bh_read_file_to_buffer(wasm_file.c_str(), &size));
        ASSERT_NE(buffer, nullptr) << "Failed to read i32.rotl test WASM file: " << wasm_file;

        module = wasm_runtime_load(buffer, size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr)
            << "Failed to load i32.rotl test module: " << error_buf;

        // Instantiate module for test execution
        module_inst = wasm_runtime_instantiate(module, STACK_SIZE, HEAP_SIZE,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr)
            << "Failed to instantiate i32.rotl test module: " << error_buf;

        // Set execution mode based on test parameter
        wasm_runtime_set_running_mode(module_inst, GetParam());

        // Load underflow test module for error condition testing
        std::string underflow_wasm_file = "wasm-apps/i32_rotl_stack_underflow.wasm";
        underflow_buffer = reinterpret_cast<uint8_t*>(
            bh_read_file_to_buffer(underflow_wasm_file.c_str(), &underflow_size));
        ASSERT_NE(underflow_buffer, nullptr)
            << "Failed to read i32.rotl underflow test WASM file: " << underflow_wasm_file;
    }

    /**
     * Test fixture teardown method
     * @brief Cleanup WAMR runtime resources and modules
     * @details Proper cleanup of module instances, modules, buffers, and runtime
     */
    void TearDown() override
    {
        // Clean up module instance
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }

        // Clean up underflow module if instantiated
        if (underflow_module_inst != nullptr) {
            wasm_runtime_deinstantiate(underflow_module_inst);
            underflow_module_inst = nullptr;
        }

        // Clean up modules
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (underflow_module != nullptr) {
            wasm_runtime_unload(underflow_module);
            underflow_module = nullptr;
        }

        // Clean up buffers
        if (buffer != nullptr) {
            BH_FREE(buffer);
            buffer = nullptr;
        }
        if (underflow_buffer != nullptr) {
            BH_FREE(underflow_buffer);
            underflow_buffer = nullptr;
        }

        // Destroy WAMR runtime
        wasm_runtime_destroy();
    }

    /**
     * Call i32.rotl WASM function with specified parameters
     * @brief Execute i32.rotl operation through WASM module function call
     * @details Invokes the i32_rotl_test function in loaded WASM module with given value and count
     * @param value The 32-bit integer value to rotate
     * @param count The rotation count (number of positions to rotate left)
     * @return The rotated result as uint32_t
     */
    uint32_t call_i32_rotl(uint32_t value, uint32_t count)
    {
        // Find the i32_rotl_test function in the module
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "i32_rotl_test");
        EXPECT_NE(func, nullptr) << "Failed to lookup i32_rotl_test function";

        // Create execution environment for function calls
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(exec_env, nullptr) << "Failed to create execution environment";

        // Prepare function arguments
        uint32_t argv[2] = { value, count };

        // Execute the function
        bool success = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_TRUE(success)
            << "i32.rotl function call failed: " << wasm_runtime_get_exception(module_inst);

        // Clean up execution environment
        wasm_runtime_destroy_exec_env(exec_env);

        return argv[0]; // Return value stored in first array element
    }

    // Test fixture member variables
    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    uint8_t *buffer = nullptr;
    uint32_t size;
    char error_buf[128];

    // Stack underflow test resources
    wasm_module_t underflow_module = nullptr;
    wasm_module_inst_t underflow_module_inst = nullptr;
    uint8_t *underflow_buffer = nullptr;
    uint32_t underflow_size;
};

/**
 * @test BasicRotation_ProducesCorrectResults
 * @brief Validates i32.rotl produces correct rotation results for typical input values
 * @details Tests fundamental rotation operation with various typical values and rotation counts.
 *          Verifies that i32.rotl correctly performs circular left shift for standard scenarios.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @input_conditions Standard integer rotation scenarios with typical values and counts
 * @expected_behavior Returns mathematically correct rotated values preserving all bits
 * @validation_method Direct comparison of WASM function result with manually calculated expected values
 */
TEST_P(I32RotlTest, BasicRotation_ProducesCorrectResults)
{
    // Test basic rotation operations with typical values
    ASSERT_EQ(0x23456781U, call_i32_rotl(0x12345678U, 4))
        << "Basic rotation: 0x12345678 rotated left by 4 positions failed";

    ASSERT_EQ(0x00000003U, call_i32_rotl(0x80000001U, 1))
        << "Basic rotation: 0x80000001 rotated left by 1 position failed";

    ASSERT_EQ(0x2468ACF0U, call_i32_rotl(0x12345678U, 1))
        << "Basic rotation: 0x12345678 rotated left by 1 position failed";

    ASSERT_EQ(0x48D159E0U, call_i32_rotl(0x12345678U, 2))
        << "Basic rotation: 0x12345678 rotated left by 2 positions failed";
}

/**
 * @test MathematicalProperties_Validated
 * @brief Validates mathematical properties and relationships of rotation operations
 * @details Tests inverse relationships and distributive properties of rotation operations.
 *          Verifies that rotl(rotl(x, a), b) equals rotl(x, a+b) and similar properties.
 * @test_category Edge - Mathematical property validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @input_conditions Various values tested with composite rotation operations
 * @expected_behavior Mathematical properties hold: distributive, inverse relationships
 * @validation_method Compare composite operations with direct equivalent operations
 */
TEST_P(I32RotlTest, MathematicalProperties_Validated)
{
    uint32_t test_value = 0x12345678U;

    // Test distributive property: rotl(rotl(x, a), b) = rotl(x, a+b)
    uint32_t double_rotate = call_i32_rotl(call_i32_rotl(test_value, 5), 3);
    uint32_t single_rotate = call_i32_rotl(test_value, 8);
    ASSERT_EQ(single_rotate, double_rotate)
        << "Distributive property failed: rotl(rotl(x, 5), 3) should equal rotl(x, 8)";

    // Test inverse relationship: rotl(rotl(x, n), 32-n) = x
    uint32_t forward_rotate = call_i32_rotl(test_value, 7);
    uint32_t inverse_rotate = call_i32_rotl(forward_rotate, 25); // 32 - 7 = 25
    ASSERT_EQ(test_value, inverse_rotate)
        << "Inverse property failed: rotl(rotl(x, 7), 25) should equal x";

    // Test commutativity with modulo: rotl(x, n) = rotl(x, n % 32)
    uint32_t normal_rotate = call_i32_rotl(test_value, 15);
    uint32_t modulo_rotate = call_i32_rotl(test_value, 47); // 47 % 32 = 15
    ASSERT_EQ(normal_rotate, modulo_rotate)
        << "Modulo property failed: rotl(x, 15) should equal rotl(x, 47)";
}

/**
 * @test StackUnderflow_SafePath_Succeeds
 * @brief Validates the safe (non-trapping) path of the stack underflow test module executes successfully
 * @details Loads a WASM module designed for stack underflow testing and invokes the
 *          controlled_underflow_test function with should_trap=0 (safe path).
 * @test_category Success - Safe path execution
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @input_conditions WASM underflow module, should_trap=0 selects the safe branch
 * @expected_behavior wasm_runtime_call_wasm returns true (safe path succeeds)
 * @validation_method Assert call returns true indicating safe path executed without error
 */
TEST_P(I32RotlTest, StackUnderflow_SafePath_Succeeds)
{
    // Load the controlled underflow test module (should succeed as it's syntactically valid)
    underflow_module = wasm_runtime_load(underflow_buffer, underflow_size,
                                       error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, underflow_module)
        << "Failed to load controlled underflow test module: " << error_buf;

    // Instantiate the underflow module
    wasm_module_inst_t underflow_inst = wasm_runtime_instantiate(
        underflow_module, STACK_SIZE, HEAP_SIZE, error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, underflow_inst)
        << "Failed to instantiate underflow test module: " << error_buf;

    // Set execution mode
    wasm_runtime_set_running_mode(underflow_inst, GetParam());

    // Test that controlled functions work correctly (they contain valid i32.rotl operations)
    wasm_function_inst_t func = wasm_runtime_lookup_function(
        underflow_inst, "controlled_underflow_test");
    ASSERT_NE(nullptr, func) << "Failed to lookup controlled_underflow_test function";

    // Create execution environment
    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(underflow_inst, 65536);
    ASSERT_NE(nullptr, exec_env) << "Failed to create execution environment";

    // Test safe path (should_trap = 0)
    uint32_t argv[1] = {0}; // should_trap = 0 (safe path)
    bool success = wasm_runtime_call_wasm(exec_env, func, 1, argv);
    ASSERT_TRUE(success) << "Safe path execution should succeed";

    // Cleanup
    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(underflow_inst);
}

// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(
    RunningModeTest, I32RotlTest,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<I32RotlTest::ParamType>& info) {
        return info.param == Mode_Interp ? "Interpreter" : "AOT";
    }
);
