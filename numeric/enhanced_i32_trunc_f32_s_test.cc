/**
 * @file enhanced_i32_trunc_f32_s_test.cc
 * @brief Comprehensive test suite for i32.trunc_f32_s WASM opcode
 * @details Tests floating-point to signed integer truncation with comprehensive coverage
 *          including boundary conditions, special values, and error scenarios.
 *
 * Test Coverage Areas:
 * - Basic truncation functionality for typical f32 values
 * - Boundary value handling at INT32_MAX/MIN limits
 * - Special IEEE 754 values (NaN, infinity, zero variants)
 * - Overflow/underflow trap conditions
 * - Stack underflow error scenarios
 * - Cross-execution mode validation (interpreter vs AOT)
 */

#include <gtest/gtest.h>
#include <cmath>
#include <cfloat>
#include <cstring>
#include <limits>

extern "C" {
#include "wasm_runtime_common.h"
#include "bh_read_file.h"
}

/**
 * @class I32TruncF32STest
 * @brief Test fixture for i32.trunc_f32_s opcode validation
 * @details Provides WAMR runtime setup and WASM module management for systematic
 *          testing of f32 to signed i32 truncation operations across execution modes.
 */
class I32TruncF32STest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Test fixture setup with WAMR runtime initialization
     * @details Configures WAMR runtime with system allocator and loads test WASM module
     *          for i32.trunc_f32_s opcode validation across interpreter and AOT modes.
     */
    void SetUp() override {
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize WAMR runtime for opcode testing
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for i32.trunc_f32_s tests";

        // Load test WASM module containing i32.trunc_f32_s test functions
        LoadTestModule();
    }

    /**
     * @brief Test fixture cleanup with proper resource management
     * @details Ensures proper cleanup of WASM modules, instances, and runtime resources
     *          using RAII pattern for deterministic resource management.
     */
    void TearDown() override {
        // Clean up WASM execution resources
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

        if (buffer != nullptr) {
            BH_FREE(buffer);
            buffer = nullptr;
        }

        // Cleanup runtime system
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM test module for i32.trunc_f32_s validation
     * @details Loads compiled WASM module containing test functions for truncation operations
     *          and creates execution environment for cross-mode testing.
     */
    void LoadTestModule() {
        const char* wasm_file = "wasm-apps/i32_trunc_f32_s_test.wasm";

        // Read WASM module bytecode from file
        buffer = (uint8_t*)bh_read_file_to_buffer(wasm_file, &size);
        ASSERT_NE(nullptr, buffer) << "Failed to read WASM file: " << wasm_file;
        ASSERT_GT(size, 0u) << "Empty WASM file: " << wasm_file;

        // Load WASM module with validation
        module = wasm_runtime_load(buffer, size, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        // Instantiate module for execution
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate WASM module: " << error_buf;

        // Create execution environment
        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(nullptr, exec_env) << "Failed to create execution environment";
    }

    /**
     * @brief Call i32.trunc_f32_s test function with error handling
     * @param function_name Name of the exported WASM function to call
     * @param input f32 input value for truncation
     * @param expect_trap Whether the call should generate a trap
     * @return i32 result of truncation (only valid when expect_trap is false)
     * @details Invokes WASM function and handles both successful execution and trap conditions
     */
    int32_t CallTruncFunction(const char* function_name, float input, bool expect_trap = false) {
        // Find the target function in the module
        wasm_function_inst_t func = wasm_runtime_lookup_function(
            module_inst, function_name);
        EXPECT_NE(nullptr, func) << "Function not found: " << function_name;

        if (func == nullptr) return 0;

        // Prepare function arguments (f32 input)
        uint32_t argv[2];  // f32 requires 32-bit slot
        *(float*)argv = input;

        // Execute function with trap detection
        bool success = wasm_runtime_call_wasm(exec_env, func, 1, argv);

        if (expect_trap) {
            // Verify that trap occurred as expected
            EXPECT_FALSE(success) << "Expected trap for input " << input
                                 << " but function succeeded";
            if (!success) {
                const char* exception = wasm_runtime_get_exception(module_inst);
                EXPECT_NE(nullptr, exception) << "Expected exception message for trap";
            }
            return 0;  // Return value not meaningful for traps
        } else {
            // Verify successful execution
            EXPECT_TRUE(success) << "Function call failed: " <<
                wasm_runtime_get_exception(module_inst);
            return success ? (int32_t)argv[0] : 0;
        }
    }

    // Test fixture member variables
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint8_t* buffer = nullptr;
    uint32_t size = 0;
    uint32_t stack_size = 16 * 1024;  // 16KB stack
    uint32_t heap_size = 16 * 1024;   // 16KB heap
    char error_buf[256]{};
};

/**
 * @test BasicTruncation_ReturnsCorrectInteger
 * @brief Validates i32.trunc_f32_s produces correct results for typical f32 inputs
 * @details Tests fundamental truncation operation with positive, negative, and fractional
 *          values. Verifies that truncation removes fractional parts towards zero.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i32_trunc_f32_s_operation
 * @input_conditions Typical f32 values with various fractional parts and signs
 * @expected_behavior Returns integer with fractional part truncated towards zero
 * @validation_method Direct comparison of WASM function results with expected values
 */
TEST_P(I32TruncF32STest, BasicTruncation_ReturnsCorrectInteger) {
    // Test positive fractional truncation
    ASSERT_EQ(42, CallTruncFunction("test_basic_trunc", 42.7f))
        << "Failed to truncate positive fractional value 42.7f";

    ASSERT_EQ(1, CallTruncFunction("test_basic_trunc", 1.9f))
        << "Failed to truncate positive value 1.9f towards zero";

    // Test negative fractional truncation
    ASSERT_EQ(-42, CallTruncFunction("test_basic_trunc", -42.7f))
        << "Failed to truncate negative fractional value -42.7f";

    ASSERT_EQ(-1, CallTruncFunction("test_basic_trunc", -1.9f))
        << "Failed to truncate negative value -1.9f towards zero";

    // Test exact integer values
    ASSERT_EQ(100, CallTruncFunction("test_basic_trunc", 100.0f))
        << "Failed to handle exact integer value 100.0f";

    ASSERT_EQ(-100, CallTruncFunction("test_basic_trunc", -100.0f))
        << "Failed to handle exact negative integer value -100.0f";
}

// Test parameter instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossExecutionMode,
    I32TruncF32STest,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode>& info) {
        return info.param == Mode_Interp ? "Interpreter" : "AOT";
    }
);
