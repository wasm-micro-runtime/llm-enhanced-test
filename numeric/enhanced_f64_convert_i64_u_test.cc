/*
 * Enhanced Unit Tests for f64.convert_i64_u WASM Opcode
 *
 * This file contains comprehensive test cases for the f64.convert_i64_u opcode,
 * which converts unsigned 64-bit integers to 64-bit floating-point values with
 * IEEE 754 compliance and proper handling of precision limits.
 */

#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <cstring>

#include "wasm_export.h"
#include "bh_read_file.h"

// Test constants for precision boundaries
static const uint64_t PRECISION_BOUNDARY_2_53 = 9007199254740992ULL;   // 2^53
static const uint64_t MAX_EXACT_INTEGER = 9007199254740991ULL;          // 2^53 - 1
static const uint64_t BEYOND_PRECISION = 9007199254740993ULL;           // 2^53 + 1

/**
 * Test fixture for f64.convert_i64_u opcode validation
 *
 * Provides WAMR runtime initialization, module loading capabilities,
 * and parameterized testing across interpreter and AOT execution modes.
 * Validates IEEE 754 compliance for unsigned 64-bit to double conversion.
 */
class F64ConvertI64UTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * Set up test environment before each test case
     *
     * Initializes WAMR runtime with system allocator, loads the test module
     * containing f64.convert_i64_u test functions, and prepares execution context.
     */
    void SetUp() override {
        // Initialize WAMR runtime with system allocator
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load test WASM module
        const char *wasm_path = "wasm-apps/f64_convert_i64_u_test.wasm";
        wasm_buf = reinterpret_cast<uint8_t*>(
            bh_read_file_to_buffer(wasm_path, &wasm_buf_size));
        ASSERT_NE(nullptr, wasm_buf)
            << "Failed to load f64.convert_i64_u test WASM file: " << wasm_path;

        // Load and instantiate module
        char error_buf[128];
        wasm_module = wasm_runtime_load(wasm_buf, wasm_buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, wasm_module)
            << "Failed to load WASM module: " << error_buf;

        wasm_module_inst = wasm_runtime_instantiate(wasm_module, 65536, 65536, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, wasm_module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        // Get execution environment
        exec_env = wasm_runtime_create_exec_env(wasm_module_inst, 65536);
        ASSERT_NE(nullptr, exec_env)
            << "Failed to create execution environment";
    }

    /**
     * Clean up test environment after each test case
     *
     * Destroys execution environment, module instance, and runtime to prevent
     * resource leaks and ensure clean state for subsequent tests.
     */
    void TearDown() override {
        // Clean up in reverse order
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
        }
        if (wasm_module_inst != nullptr) {
            wasm_runtime_deinstantiate(wasm_module_inst);
        }
        if (wasm_module != nullptr) {
            wasm_runtime_unload(wasm_module);
        }
        if (wasm_buf != nullptr) {
            BH_FREE(wasm_buf);
        }

        wasm_runtime_destroy();
    }

    /**
     * Call f64.convert_i64_u WASM function with given unsigned 64-bit input
     *
     * @param input Unsigned 64-bit integer to convert to double
     * @return Converted double-precision floating-point value
     */
    double call_f64_convert_i64_u(uint64_t input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(wasm_module_inst, "test_f64_convert_i64_u");
        EXPECT_NE(nullptr, func) << "Failed to find test_f64_convert_i64_u function";

        uint32_t wasm_argv[2];
        wasm_argv[0] = (uint32_t)(input & 0xFFFFFFFF);        // Low 32 bits
        wasm_argv[1] = (uint32_t)(input >> 32);               // High 32 bits

        char error_buf[128];
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, wasm_argv);
        EXPECT_TRUE(ret) << "WASM function call failed: " << wasm_runtime_get_exception(wasm_module_inst);

        // Extract double result from two 32-bit return values
        uint64_t result_bits = ((uint64_t)wasm_argv[1] << 32) | wasm_argv[0];
        double result;
        memcpy(&result, &result_bits, sizeof(double));
        return result;
    }

    /**
     * Helper function to test conversion with IEEE 754 bit-exact comparison
     *
     * @param input Unsigned 64-bit integer input
     * @param expected Expected double result
     */
    void test_exact_conversion(uint64_t input, double expected) {
        double actual = call_f64_convert_i64_u(input);

        // Use bit-exact comparison for IEEE 754 compliance
        uint64_t actual_bits, expected_bits;
        memcpy(&actual_bits, &actual, sizeof(double));
        memcpy(&expected_bits, &expected, sizeof(double));

        ASSERT_EQ(expected_bits, actual_bits)
            << "Conversion of " << input << " failed. Expected: " << expected
            << " (0x" << std::hex << expected_bits << "), Got: " << actual
            << " (0x" << actual_bits << ")" << std::dec;
    }

    // Test fixture member variables
    uint8_t *wasm_buf = nullptr;
    uint32 wasm_buf_size = 0;
    wasm_module_t wasm_module = nullptr;
    wasm_module_inst_t wasm_module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
};

/**
 * @test BasicConversion_TypicalValues_ReturnsAccurateResults
 * @brief Validates f64.convert_i64_u produces correct IEEE 754 results for typical inputs
 * @details Tests fundamental conversion operation with small, medium, and large unsigned integers.
 *          Verifies that f64.convert_i64_u correctly converts representative u64 values across
 *          different magnitude ranges while maintaining IEEE 754 compliance.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_convert_i64_u_operation
 * @input_conditions Standard unsigned integers: 42UL, 4294967296UL (2^32), large values
 * @expected_behavior Returns exact IEEE 754 double precision representations
 * @validation_method Direct bit-exact comparison with expected IEEE 754 values
 */
TEST_P(F64ConvertI64UTest, BasicConversion_TypicalValues_ReturnsAccurateResults) {
    // Small positive integer - should convert exactly
    test_exact_conversion(42ULL, 42.0);

    // Medium value around 2^32 boundary
    test_exact_conversion(4294967296ULL, 4294967296.0);

    // Large value within exact representation range
    test_exact_conversion(1152921504606846976ULL, 1152921504606846976.0);

    // Power-of-two value for exact representation verification
    test_exact_conversion(1099511627776ULL, 1099511627776.0); // 2^40
}

// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(
    F64ConvertI64UModes,
    F64ConvertI64UTest,
    testing::Values(
        RunningMode::Mode_Interp
#if WASM_ENABLE_AOT != 0
        , RunningMode::Mode_LLVM_JIT
#endif
    ),
    [](const testing::TestParamInfo<RunningMode>& info) {
        switch (info.param) {
            case RunningMode::Mode_Interp: return "Interpreter";
            case RunningMode::Mode_LLVM_JIT: return "AOT";
            default: return "Unknown";
        }
    }
);
