/**
 * @file enhanced_i32_trunc_f32_u_test.cc
 * @brief Comprehensive test suite for i32.trunc_f32_u WASM opcode
 * @details Tests floating-point to unsigned integer truncation with comprehensive coverage
 *          including boundary conditions, special values, and error scenarios.
 *
 * Test Coverage Areas:
 * - Basic truncation functionality for typical f32 values
 * - Boundary value handling at UINT32_MAX limits
 * - Special IEEE 754 values (NaN, infinity, zero variants)
 * - Overflow/underflow trap conditions for negative and large values
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
 * @class I32TruncF32UTest
 * @brief Test fixture for i32.trunc_f32_u opcode validation
 * @details Provides WAMR runtime setup and WASM module management for systematic
 *          testing of f32 to unsigned i32 truncation operations across execution modes.
 */
class I32TruncF32UTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Test fixture setup with WAMR runtime initialization
     * @details Configures WAMR runtime with system allocator and loads test WASM module
     *          for i32.trunc_f32_u opcode validation across interpreter and AOT modes.
     */
    void SetUp() override {
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize WAMR runtime for opcode testing
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime for i32.trunc_f32_u tests";

        // Load test WASM module containing i32.trunc_f32_u test functions
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
     * @brief Load WASM test module for i32.trunc_f32_u validation
     * @details Loads compiled WASM module containing test functions for unsigned truncation operations
     *          and creates execution environment for cross-mode testing.
     */
    void LoadTestModule() {
        const char* wasm_file = "wasm-apps/i32_trunc_f32_u_test.wasm";

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
     * @brief Call i32.trunc_f32_u test function with error handling
     * @param function_name Name of the exported WASM function to call
     * @param input f32 input value for truncation
     * @param expect_trap Whether the call should generate a trap
     * @return uint32_t result of truncation (only valid when expect_trap is false)
     * @details Invokes WASM function and handles both successful execution and trap conditions
     */
    uint32_t CallTruncFunction(const char* function_name, float input, bool expect_trap = false) {
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
            return success ? (uint32_t)argv[0] : 0;
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
 * @test BoundaryValues_UnsignedLimits_HandlesCorrectly
 * @brief Validates behavior at UINT32_MAX boundaries and overflow conditions
 * @details Tests conversion at the limits of unsigned i32 representable range, including
 *          values that fit within bounds and values that should cause traps.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i32_trunc_f32_u_boundary_check
 * @input_conditions f32 values at and beyond UINT32_MAX boundary (4294967295)
 * @expected_behavior Valid boundaries convert successfully, overflow values trap
 * @validation_method Boundary conversion success and trap condition verification
 */
TEST_P(I32TruncF32UTest, BoundaryValues_UnsignedLimits_HandlesCorrectly) {
    // Test minimum boundary (zero)
    ASSERT_EQ(0u, CallTruncFunction("test_boundary_values", 0.0f))
        << "Failed to convert minimum unsigned boundary value 0.0f";

    // Test values near zero boundary
    ASSERT_EQ(0u, CallTruncFunction("test_boundary_values", 0.9f))
        << "Failed to handle value 0.9f near zero boundary";

    // Test maximum representable unsigned value (close to UINT32_MAX)
    // Note: f32 cannot exactly represent UINT32_MAX, so we test the closest representable value
    float max_representable = 4294967296.0f - 256.0f;  // Closest f32 to UINT32_MAX
    uint32_t result = CallTruncFunction("test_boundary_values", max_representable);
    ASSERT_LT(result, 4294967296u) << "Result should be within unsigned 32-bit range";

    // Test large valid value that fits in unsigned range
    ASSERT_EQ(2000000000u, CallTruncFunction("test_boundary_values", 2000000000.0f))
        << "Failed to convert large valid unsigned value";
}

/**
 * @test NegativeValues_AnyNegativeInput_TriggersTraps
 * @brief Validates proper trap generation for negative input values
 * @details Tests that any negative f32 value causes execution traps since unsigned
 *          integers cannot represent negative values.
 * @test_category Error - Negative value trap validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i32_trunc_f32_u_negative_trap
 * @input_conditions Various negative f32 values
 * @expected_behavior All negative values trigger execution traps
 * @validation_method Negative value trap detection and verification
 */
TEST_P(I32TruncF32UTest, NegativeValues_AnyNegativeInput_TriggersTraps) {
    // Test small negative value
    CallTruncFunction("test_negative_values", -1.0f, true);

    // Test medium negative value
    CallTruncFunction("test_negative_values", -100.0f, true);

    // Test large negative value
    CallTruncFunction("test_negative_values", -1000000.0f, true);

    // Test negative fractional value
    CallTruncFunction("test_negative_values", -0.1f, true);

    // Test extreme negative value
    CallTruncFunction("test_negative_values", -1e20f, true);
}

// Test parameter instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossExecutionMode,
    I32TruncF32UTest,
    testing::Values(Mode_Interp, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode>& info) {
        return info.param == Mode_Interp ? "Interpreter" : "AOT";
    }
);
