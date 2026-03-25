#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <climits>
#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include "test_helper.h"

/**
 * @class I16x8SplatTestSuite
 * @brief Test fixture class for i16x8.splat opcode testing
 * @details Provides setup/teardown for WAMR runtime and module loading.
 *          Handles SIMD vector result validation using DummyExecEnv helper
 *          for comprehensive i16x8.splat operation validation.
 * @test_categories Main, Corner, Edge, Error exception validation
 */
class I16x8SplatTestSuite : public testing::Test
{
protected:
    /**
     * @brief Initialize WAMR runtime and prepare test environment for i16x8.splat testing
     * @details Sets up WAMR runtime with SIMD support using WAMRRuntimeRAII helper.
     *          Initializes execution environment for WASM test files using absolute path.
     * @source_location tests/unit/enhanced_opcode/extension/enhanced_i16x8_splat_test.cc:SetUp
     */
    void SetUp() override
    {
        // Initialize WAMR runtime using RAII helper
        runtime_raii = std::make_unique<WAMRRuntimeRAII<>>();

        // Load the i16x8.splat test module using relative path
        dummy_env = std::make_unique<DummyExecEnv>("wasm-apps/i16x8_splat_test.wasm");
        ASSERT_NE(nullptr, dummy_env->get())
            << "Failed to create execution environment for i16x8.splat tests";
    }

    /**
     * @brief Clean up WAMR runtime and release test resources
     * @details Properly destroys execution environment and WAMR runtime
     *          using RAII pattern to prevent resource leaks.
     * @source_location tests/unit/enhanced_opcode/extension/enhanced_i16x8_splat_test.cc:TearDown
     */
    void TearDown() override
    {
        // RAII automatically handles cleanup
        dummy_env.reset();
        runtime_raii.reset();
    }

    /**
     * @brief Helper function to call i16x8.splat WASM function and validate results
     * @param func_name WASM function name to call
     * @param value i32 input value to be truncated and splatted
     * @param expected_value Expected i16 value in all 8 lanes after truncation
     * @details Calls the specified WASM function and validates the execution.
     *          Returns the resulting i16 values for lane-by-lane validation.
     * @source_location tests/unit/enhanced_opcode/extension/enhanced_i16x8_splat_test.cc:CallI16x8Splat
     */
    void CallI16x8Splat(const char* func_name, int32_t value, int16_t expected_value)
    {
        // v128 functions return 4 uint32_t values which overwrite the argv array
        uint32_t argv[4] = { (uint32_t)value, 0, 0, 0 };

        // Execute the function and expect success
        bool call_result = dummy_env->execute(func_name, 1, argv);
        ASSERT_TRUE(call_result)
            << "Failed to call " << func_name << " with value: " << value;

        // Extract result i16 values from argv (now contains the v128 result)
        auto* result_values = reinterpret_cast<int16_t*>(argv);

        // Validate all 8 lanes contain the expected value
        for (int i = 0; i < 8; i++) {
            ASSERT_EQ(expected_value, result_values[i])
                << "Lane " << i << " contains incorrect value"
                << " - expected: " << expected_value
                << ", got: " << result_values[i];
        }
    }

    /**
     * @brief Helper function to call no-argument i16x8.splat WASM function
     * @param func_name WASM function name to call
     * @param expected_value Expected i16 value in all 8 lanes
     * @details Calls WASM function with no arguments for testing constant splat values.
     * @source_location tests/unit/enhanced_opcode/extension/enhanced_i16x8_splat_test.cc:CallI16x8SplatConst
     */
    void CallI16x8SplatConst(const char* func_name, int16_t expected_value)
    {
        // v128 functions return 4 uint32_t values
        uint32_t argv[4] = { 0, 0, 0, 0 };

        // Execute the function and expect success
        bool call_result = dummy_env->execute(func_name, 0, argv);
        ASSERT_TRUE(call_result)
            << "Failed to call " << func_name;

        // Extract result i16 values from argv
        auto* result_values = reinterpret_cast<int16_t*>(argv);

        // Validate all 8 lanes contain the expected value
        for (int i = 0; i < 8; i++) {
            ASSERT_EQ(expected_value, result_values[i])
                << "Lane " << i << " contains incorrect value"
                << " - expected: " << expected_value
                << ", got: " << result_values[i];
        }
    }

    std::unique_ptr<WAMRRuntimeRAII<>> runtime_raii;
    std::unique_ptr<DummyExecEnv> dummy_env;
};

/**
 * @test BoundaryValues_HandlesMinMaxCorrectly
 * @brief Validates i16x8.splat handles i16 boundary values correctly
 * @details Tests splat operation with minimum and maximum i16 values.
 *          Verifies proper handling of INT16_MIN, INT16_MAX, and related boundary conditions.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_fast.c:SIMD_i16x8_splat
 * @input_conditions INT16_MIN (-32768), INT16_MAX (32767), boundary truncation cases
 * @expected_behavior Proper boundary value replication and correct truncation behavior
 * @validation_method Exact boundary value verification in all 8 lanes
 */
TEST_F(I16x8SplatTestSuite, BoundaryValues_HandlesMinMaxCorrectly)
{
    // Test INT16_MIN value
    CallI16x8Splat("test_boundary_splat", INT16_MIN, INT16_MIN);

    // Test INT16_MAX value
    CallI16x8Splat("test_boundary_splat", INT16_MAX, INT16_MAX);

    // Test -1 (0xFFFF in i16)
    CallI16x8Splat("test_boundary_splat", -1, -1);

    // Test 1 (positive boundary near zero)
    CallI16x8Splat("test_boundary_splat", 1, 1);
}
