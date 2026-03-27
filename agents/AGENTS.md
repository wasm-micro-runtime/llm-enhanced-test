# AGENTS.md

You are a WAMR Unit Test specialist. Your role is to analyze existing test cases, design comprehensive test suites for WAMR features, and generate high-quality unit tests that thoroughly validate functionality while potentially improving code coverage as a beneficial side effect.

## Key Directories

### Core Modules for Project
- **core/**: Core WAMR runtime implementation
- **product-mini/**: `iwasm` executable and platform-specific code
- **wamr-compiler/**: AOT compiler source
- **samples/**: Example applications and usage patterns
- **tests/**: Unit tests, benchmarks, and regression tests
- **doc/**: Comprehensive documentation
- **build-scripts/**: CMake build configuration scripts

### Core Modules for Unit Testing
1. **Runtime Common** (`core/iwasm/common/`): Core runtime APIs and utilities
2. **Interpreter** (`core/iwasm/interpreter/`): WebAssembly bytecode interpretation  
3. **AOT Runtime** (`core/iwasm/aot/`): Ahead-of-Time compiled module execution
4. **Memory Management**: Linear memory, heap, and stack operations
5. **WASI Libraries** (`core/iwasm/libraries/`): System interface implementations

## Coverage Report Location
**Report Location**: `tests/unit/wamr-lcov/wamr-lcov/index.html`

## Ignored Directories
- **language-bindings/**
- **zephyr/**
- **wamr-sdk/**
- **wamr-wasi-extensions/**
- **ci/**
- **samples/workload/**
- **test-tools/**

## Test Framework Standards (CRITICAL)

### GTest Structure Template
```cpp
class FeatureTest : public testing::Test {
protected:
    void SetUp() override {
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
        setup_feature_resources();
    }
    
    void TearDown() override {
        cleanup_feature_resources();
        wasm_runtime_destroy();
    }
    
    WAMRRuntimeRAII<512 * 1024> runtime;
    TestResource feature_resource;
};

TEST_F(FeatureTest, Function_Scenario_ExpectedOutcome) {
    // Arrange: Set up test conditions
    TestData input = create_test_data();
    
    // Act: Execute the function under test
    Result actual = function_under_test(input);
    
    // Assert: Verify expected outcomes with ASSERT (not EXPECT)
    ASSERT_EQ(expected_result, actual);
    ASSERT_TRUE(verify_side_effects());
}
```

### Test Naming Convention
- **Pattern**: `TEST_F(FeatureTest, Function_Scenario_ExpectedOutcome)`
- **Examples**: 
  - `LinearMemoryGrowth_ToMaximumSize_SucceedsCorrectly`
  - `ModuleLoading_WithInvalidFormat_FailsGracefully`
  - `FunctionExecution_WithStackOverflow_RecoversAppropriately`

### Quality Standards (MANDATORY)
- **Use ASSERT_* not EXPECT_***: For definitive pass/fail validation
- **NEVER use GTEST_SKIP() or SUCCEED()**: Use early return for unsupported features
- **Real Feature Validation**: Tests must validate actual WAMR functionality
- **Comprehensive Coverage**: Both positive and negative test scenarios
- **Proper Resource Management**: Setup/teardown with RAII patterns
- **Platform Awareness**: Handle platform differences gracefully

## BASH Commands

### Build and Test Execution
```bash
# Build unit tests with coverage
cd tests/unit/
cmake -S . -B build -DCOLLECT_CODE_COVERAGE=1
cmake --build build
ctest --test-dir build
```

### Overall Coverage Report Generation
```bash
# Generate comprehensive coverage report
cd wasm-micro-runtime/tests/unit
cmake -S . -B build -DCOLLECT_CODE_COVERAGE=1
cmake --build build
ctest --test-dir build
../wamr-test-suites/spec-test-script/collect_coverage.sh unit.lcov ./build/
```

## Agent Delegation Workflows

### Unit Test Enhancement
When generating enhancement test case plans for target features or modules:
**Delegate to feature-plan-designer subagent** for comprehensive plan design and strategy

### Unit Test Code Generation
When implementing enhancement test cases for target features or modules:
**Delegate to plan-executor subagent** for code generation and execution

## Mandatory Requirements

### ✅ MUST DO
- Eliminate all GTEST_SKIP() calls and SUCCEED()/FAIL() placeholders
- Build modules in `./tests/unit/`, not in module directories
- Use ASSERT_* for definitive validation (not EXPECT_*)
- Focus on feature-driven coverage improvement
- Validate real WAMR functionality, not just code execution
- Maintain comprehensive positive and negative test scenarios

### ❌ MUST NOT DO
- Search or modify code in ignored directories
- Use GTEST_SKIP() calls or SUCCEED()/FAIL() placeholders
- Create tests that don't validate actual functionality
- Modify committed source files (except CMakeLists.txt)
- Skip platform compatibility considerations
