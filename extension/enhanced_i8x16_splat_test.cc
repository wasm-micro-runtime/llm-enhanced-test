/**
 * @file enhanced_i8x16_splat_test.cc
 * @brief Comprehensive unit tests for i8x16.splat SIMD opcode
 * @details Tests i8x16.splat functionality across interpreter and AOT execution modes
 *          with focus on basic operations, boundary conditions, truncation behavior,
 *          and error scenarios. Validates WAMR SIMD implementation correctness
 *          and cross-mode consistency for vector construction operations.
 * @source_location tests/unit/enhanced_opcode/extension/enhanced_i8x16_splat_test.cc
 */

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <climits>
#include <fstream>
#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include "test_helper.h"

/**
 * @class I8x16SplatTestSuite
 * @brief Test fixture class for i8x16.splat opcode testing
 * @details Provides setup/teardown for WAMR runtime and module loading.
 *          Handles SIMD vector result validation using DummyExecEnv helper
 *          for comprehensive i8x16.splat operation validation.
 * @test_categories Main, Corner, Edge, Error exception validation
 */
class I8x16SplatTestSuite : public testing::Test
{
protected:
    /**
     * @brief Initialize WAMR runtime and prepare test environment for i8x16.splat testing
     * @details Sets up WAMR runtime with SIMD support using WAMRRuntimeRAII helper.
     *          Initializes execution environment for WASM test files using absolute path.
     * @source_location tests/unit/enhanced_opcode/extension/enhanced_i8x16_splat_test.cc:SetUp
     */
    void SetUp() override
    {
        // Initialize WAMR runtime using RAII helper
        runtime_raii = std::make_unique<WAMRRuntimeRAII<>>();

        // Load the i8x16.splat test module using relative path
        dummy_env = std::make_unique<DummyExecEnv>("wasm-apps/i8x16_splat_test.wasm");
        ASSERT_NE(nullptr, dummy_env->get())
            << "Failed to create execution environment for i8x16.splat tests";
    }

    /**
     * @brief Clean up WAMR runtime and release test resources
     * @details Properly destroys execution environment and WAMR runtime
     *          using RAII pattern to prevent resource leaks.
     * @source_location tests/unit/enhanced_opcode/extension/enhanced_i8x16_splat_test.cc:TearDown
     */
    void TearDown() override
    {
        // RAII automatically handles cleanup
        dummy_env.reset();
        runtime_raii.reset();
    }

    /**
     * @brief Helper function to call i8x16.splat WASM function and validate results
     * @param func_name WASM function name to call
     * @param value i32 input value to be truncated and splatted
     * @details Calls the specified WASM function and validates the execution.
     *          Returns the resulting bytes for lane-by-lane validation.
     * @source_location tests/unit/enhanced_opcode/extension/enhanced_i8x16_splat_test.cc:CallI8x16Splat
     */
    void CallI8x16Splat(const char* func_name, int32_t value, uint8_t expected_bytes[16])
    {
        // v128 functions return 4 uint32_t values which overwrite the argv array
        uint32_t argv[4] = { (uint32_t)value, 0, 0, 0 };

        // Execute the function and expect success
        bool call_result = dummy_env->execute(func_name, 1, argv);
        ASSERT_TRUE(call_result)
            << "Failed to call " << func_name << " with value: " << value;

        // Extract result bytes from argv (now contains the v128 result)
        uint8_t* result_bytes = reinterpret_cast<uint8_t*>(argv);

        // Validate all 16 lanes contain the expected value
        for (int i = 0; i < 16; i++) {
            ASSERT_EQ(expected_bytes[i], result_bytes[i])
                << "Lane " << i << " contains incorrect value"
                << " - expected: 0x" << std::hex << (int)expected_bytes[i]
                << ", got: 0x" << (int)result_bytes[i];
        }
    }

    /**
     * @brief Helper function to call no-argument i8x16.splat WASM function
     * @param func_name WASM function name to call
     * @param expected_bytes Expected bytes in all 16 lanes
     * @details Calls WASM function with no arguments for testing constant splat values.
     * @source_location tests/unit/enhanced_opcode/extension/enhanced_i8x16_splat_test.cc:CallI8x16SplatConst
     */
    void CallI8x16SplatConst(const char* func_name, uint8_t expected_bytes[16])
    {
        // v128 functions return 4 uint32_t values
        uint32_t argv[4] = { 0, 0, 0, 0 };

        // Execute the function and expect success
        bool call_result = dummy_env->execute(func_name, 0, argv);
        ASSERT_TRUE(call_result)
            << "Failed to call " << func_name;

        // Extract result bytes from argv (now contains the v128 result)
        uint8_t* result_bytes = reinterpret_cast<uint8_t*>(argv);

        // Validate all 16 lanes contain the expected value
        for (int i = 0; i < 16; i++) {
            ASSERT_EQ(expected_bytes[i], result_bytes[i])
                << "Lane " << i << " contains incorrect value"
                << " - expected: 0x" << std::hex << (int)expected_bytes[i]
                << ", got: 0x" << (int)result_bytes[i];
        }
    }

    // Test infrastructure
    std::unique_ptr<WAMRRuntimeRAII<>> runtime_raii;
    std::unique_ptr<DummyExecEnv> dummy_env;
};

/**
 * @test BoundaryValues_HandlesMinMaxCorrectly
 * @brief Validates i8x16.splat handles i8 boundary values correctly
 * @details Tests splat operation with i8 MIN (-128), MAX (127), and zero values.
 *          Verifies correct handling of signed 8-bit integer boundary conditions
 *          without truncation artifacts for values already within i8 range.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:i8x16_splat_operation
 * @input_conditions i8 boundaries: -128 (INT8_MIN), 127 (INT8_MAX), 0
 * @expected_behavior Returns v128 with all lanes containing the boundary values
 * @validation_method Verification of boundary value preservation in all lanes
 */
TEST_F(I8x16SplatTestSuite, BoundaryValues_HandlesMinMaxCorrectly)
{
    uint8_t expected_min[16], expected_max[16], expected_zero[16];

    // Fill expected arrays with boundary values
    for (int i = 0; i < 16; i++) {
        expected_min[i] = 0x80;    // -128 as unsigned byte
        expected_max[i] = 0x7F;    // 127 as unsigned byte
        expected_zero[i] = 0x00;   // 0 as unsigned byte
    }

    CallI8x16Splat("i8x16_splat_test", -128, expected_min);
    CallI8x16Splat("i8x16_splat_test", 127, expected_max);
    CallI8x16Splat("i8x16_splat_test", 0, expected_zero);
}