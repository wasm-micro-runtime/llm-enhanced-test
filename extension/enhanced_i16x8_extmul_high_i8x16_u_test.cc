#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include "test_helper.h"

class I16x8ExtmulHighI8x16UTestSuite : public testing::Test
{
protected:
    /**
     * @brief Set up test environment for each test case
     * @details Initializes WAMR runtime using WAMRRuntimeRAII helper and prepares
     *          test module for i16x8.extmul_high_i8x16_u opcode validation
     */
    void SetUp() override
    {
        // Initialize WAMR runtime using RAII helper
        runtime_raii = std::make_unique<WAMRRuntimeRAII<512 * 1024>>();

        // Load the i16x8.extmul_high_i8x16_u test module
        dummy_env = std::make_unique<DummyExecEnv>("wasm-apps/i16x8_extmul_high_i8x16_u_test.wasm");
        ASSERT_NE(nullptr, dummy_env->get())
            << "Failed to create execution environment for i16x8.extmul_high_i8x16_u tests";
    }

    /**
     * @brief Clean up test environment after each test case
     * @details Properly destroys execution environment and WAMR runtime
     *          using RAII pattern to prevent resource leaks
     */
    void TearDown() override
    {
        // RAII handles cleanup automatically
        dummy_env.reset();
        runtime_raii.reset();
    }

    /**
     * @brief Helper function to call i16x8.extmul_high_i8x16_u test function
     * @param a_bytes Array of 16 i8 values for first v128 vector
     * @param b_bytes Array of 16 i8 values for second v128 vector
     * @param result_i16 Output array of 8 i16 values for result
     * @details Calls WASM function that performs i16x8.extmul_high_i8x16_u on high lanes (8-15)
     */
    void call_i16x8_extmul_high_i8x16_u(const int8_t a_bytes[16], const int8_t b_bytes[16],
                                         int16_t result_i16[8])
    {
        // v128 functions take two v128 parameters (8 uint32_t values total)
        // and return one v128 (4 uint32_t values)
        uint32_t argv[8]; // 8 uint32_t = 2 v128 inputs

        // Pack input vectors into argv - each v128 is 4 uint32_t values
        // First v128 (argv[0-3])
        memcpy(&argv[0], a_bytes, 16);
        // Second v128 (argv[4-7])
        memcpy(&argv[4], b_bytes, 16);

        bool call_result = dummy_env->execute("test_i16x8_extmul_high_i8x16_u", 8, argv);
        ASSERT_TRUE(call_result) << "Failed to call WASM function: " << dummy_env->get_exception();

        // Extract v128 result - the return value overwrites argv starting from index 0
        // Cast the argv array directly to get the result bytes
        auto *result_bytes = reinterpret_cast<uint8_t*>(argv);

        memcpy(result_i16, result_bytes, 8 * sizeof(int16_t));
    }

private:
    std::unique_ptr<WAMRRuntimeRAII<512 * 1024>> runtime_raii;
    std::unique_ptr<DummyExecEnv> dummy_env;
};

/**
 * @test BoundaryValues_HandlesMinMaxCorrectly
 * @brief Tests boundary conditions with 0 and 255 (max unsigned i8) values
 * @details Validates correct handling of minimum and maximum unsigned i8 values,
 *          including maximum possible product (255 * 255 = 65025).
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i16x8_extmul_high_i8x16_u_boundary_handling
 * @input_conditions Mix of minimum (0) and maximum (255) unsigned values
 * @expected_behavior Correct handling of boundary multiplications including max product
 * @validation_method Verify boundary value multiplication results
 */
TEST_F(I16x8ExtmulHighI8x16UTestSuite, BoundaryValues_HandlesMinMaxCorrectly)
{
    // Test with boundary values: 0 and 255 (max unsigned i8)
    int8_t vec_a[16] = {0, 0, 0, 0, 0, 0, 0, 0,                      // low lanes (ignored)
                        0, 1, -1, -1, 0, -1, 127, -128};             // high lanes: 0,1,255,255,0,255,127,128
    int8_t vec_b[16] = {0, 0, 0, 0, 0, 0, 0, 0,                      // low lanes (ignored)
                        -1, -1, -1, -2, 100, 50, 2, 2};              // high lanes: 255,255,255,254,100,50,2,2

    int16_t result[8];
    call_i16x8_extmul_high_i8x16_u(vec_a, vec_b, result);

    // Verify boundary value multiplications (treating i8 as unsigned)
    // Note: Results are stored as i16, so values > 32767 will appear negative when cast to signed
    ASSERT_EQ(result[0], 0)     << "Lane 0: 0 * 255 should equal 0";
    ASSERT_EQ(result[1], 255)   << "Lane 1: 1 * 255 should equal 255";
    ASSERT_EQ((uint16_t)result[2], 65025) << "Lane 2: 255 * 255 should equal 65025 (max product)";
    ASSERT_EQ((uint16_t)result[3], 64770) << "Lane 3: 255 * 254 should equal 64770";
    ASSERT_EQ(result[4], 0)     << "Lane 4: 0 * 100 should equal 0";
    ASSERT_EQ((uint16_t)result[5], 12750) << "Lane 5: 255 * 50 should equal 12750";
    ASSERT_EQ(result[6], 254)   << "Lane 6: 127 * 2 should equal 254";
    ASSERT_EQ(result[7], 256)   << "Lane 7: 128 * 2 should equal 256";
}
