# Test Review Summary: enhanced_aot_emit_conversion_test.cc

## Redundancy Cleanup (from check_redundant_tests.js)

- **Original tests:** 50
- **Identified (redundant):** 44
- **Remaining tests (useful):** 6

### Redundant Test Cases (deleted in PHASE 1.5)
| Test Case | Reason | Status |
|-----------|--------|--------|
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_wrap_i64_Success_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_wrap_i64_LLVMBuildError_ReturnsFalse` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_wrap_i64_MultipleConversions_ExecutesAll` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_trunc_f32_SignedNonSaturating_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_trunc_f32_UnsignedNonSaturating_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_trunc_f32_SignedSaturating_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_trunc_f32_UnsignedSaturating_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_trunc_f64_SignedNonSaturating_DirectMode_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_trunc_f64_UnsignedNonSaturating_DirectMode_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_trunc_f64_SignedSaturating_DirectMode_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_trunc_f64_UnsignedSaturating_DirectMode_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_extend_i32_SignedExtension_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_extend_i32_UnsignedExtension_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_extend_i32_CombinedOperations_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_extend_i64_Extend8S_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_extend_i64_Extend16S_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_extend_i64_Extend32S_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_extend_i64_CombinedOperations_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_extend_i32_Extend8S_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_extend_i32_Extend16S_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i32_extend_i32_CombinedOperations_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f32_SignedNonSaturating_DirectMode_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f32_UnsignedNonSaturating_DirectMode_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f32_SignedSaturating_DirectMode_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f32_UnsignedSaturating_DirectMode_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f32_CombinedOperations_AllPaths_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f64_Signed_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f64_Unsigned_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f64_SignedSaturating_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f64_UnsignedSaturating_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f32_convert_i32_Signed_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f32_convert_i32_Unsigned_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f32_convert_i64_UnsignedConversion_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f32_convert_i64_UnsignedWithIntrinsics_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f32_convert_i64_CombinedOperations_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f32_demote_f64_Success_StandardPath` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f32_demote_f64_Success_IntrinsicPath` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f32_demote_f64_IntrinsicEnabled_TestIntrinsicPath` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f64_convert_i64_UnsignedStandardPath_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f64_convert_i64_SignedIntrinsicPath_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f64_convert_i64_UnsignedIntrinsicPath_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f64_promote_f32_StandardLLVMPath_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f64_promote_f32_IntrinsicPath_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitConversionTest.aot_compile_op_f64_promote_f32_ComplexOperations_ReturnsTrue` | No incremental coverage contribution | Deleted |

---

## Test Case [1/6]: EnhancedAotEmitConversionTest.aot_compile_op_i64_trunc_f32_IndirectMode_IntrinsicCapability_ReturnsTrue

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_conversion_test.cc`
**Start line**: 80
**Parameterized**: No

### Coverage
- Lines: 7.2% (2660/36711)
- Functions: 9.9% (221/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_i64_trunc_f32`
**Intended scenario**: Load a wasm module containing i64.trunc_f32 instructions, compile with `is_indirect_mode = true` to trigger the indirect mode path through AOT compilation of i64_trunc_f32 operations. Uses `opt_level=1`, `bounds_checks=2`, `enable_bulk_memory=true`.
**Intended outcome**: `aot_compile_wasm` returns true, meaning compilation succeeds in indirect mode.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_i64_trunc_f32` in `aot_emit_conversion.c` (called 8 times -- signed/unsigned, saturating/non-saturating variants)
- `trunc_float_to_int` in `aot_emit_conversion.c` (called 4 times)
- `trunc_sat_float_to_int` in `aot_emit_conversion.c` (called 4 times)
- `aot_intrinsic_check_capability` in `aot_intrinsic.c` (called 8 times)
- `call_fcmp_intrinsic` in `aot_emit_conversion.c` (called 24 times)

**Uncovered paths** (from coverage-summary.md): `aot_create_comp_data` malloc failure, `aot_create_comp_context` allocation failures, `aot_compile_wasm` validation/JIT failures. Many branches in `aot_create_comp_context` (255 gap lines out of 426 covered+gap).

**Actual code path**: The test successfully compiles a wasm module with i64.trunc_f32 instructions in indirect mode. The compilation succeeds through the normal success path of `aot_compile_wasm`. The `aot_intrinsic_check_capability` is called to check intrinsic support for indirect mode, and both `trunc_float_to_int` and `trunc_sat_float_to_int` are exercised.

**Path type** (from coverage): SUCCESS

### Alignment: YES

The test name claims indirect mode intrinsic capability for i64_trunc_f32 returning true, and coverage confirms `aot_compile_op_i64_trunc_f32` is called with intrinsic capability checks in a successful compilation.

### Quality Screening

None

---

## Test Case [2/6]: EnhancedAotEmitConversionTest.aot_compile_op_f32_convert_i32_WithIntrinsics_ReturnsTrue

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_conversion_test.cc`
**Start line**: 135
**Parameterized**: No

### Coverage
- Lines: 6.8% (2478/36711)
- Functions: 9.7% (217/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_f32_convert_i32`
**Intended scenario**: Load a wasm module with f32.convert_i32 instructions. Set `disable_llvm_intrinsics = true` which the comment says "Enable intrinsics to trigger the intrinsic path." This is contradictory -- `disable_llvm_intrinsics=true` disables LLVM intrinsics, forcing use of constrained intrinsic calls instead.
**Intended outcome**: `aot_compile_wasm` returns true.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_f32_convert_i32` in `aot_emit_conversion.c` (called 6 times)
- `aot_intrinsic_fill_capability_flags` in `aot_intrinsic.c` (called 1 time)
- `add_intrinsic_capability` in `aot_intrinsic.c` (called 2 times)
- `aot_intrinsic_check_capability` in `aot_intrinsic.c` (called 8 times)
- `call_llvm_float_experimental_constrained_intrinsic` in `aot_emit_conversion.c` (called 1 time)
- `aot_call_llvm_intrinsic_v` / `__call_llvm_intrinsic` in `aot_llvm_extra2.cpp` (called 1 time each)

**Uncovered paths** (from coverage-summary.md): Same infrastructure failure paths as test 1. Additionally, `aot_destroy_comp_context` has more gap lines (10) including native symbol list cleanup branch.

**Actual code path**: The test compiles with `disable_llvm_intrinsics=true`, which triggers the constrained float intrinsic path through `call_llvm_float_experimental_constrained_intrinsic`. The compilation succeeds through the success path.

**Path type** (from coverage): SUCCESS

### Alignment: YES

The test name claims f32_convert_i32 with intrinsics returning true, and coverage confirms `aot_compile_op_f32_convert_i32` is called with intrinsic-related functions (`call_llvm_float_experimental_constrained_intrinsic`, `aot_intrinsic_check_capability`) exercised. The comment about "enable intrinsics" is misleading (it actually disables LLVM intrinsics, forcing constrained intrinsic path), but the test name and actual behavior are aligned.

### Quality Screening

None

---

## Test Case [3/6]: EnhancedAotEmitConversionTest.aot_compile_op_f32_convert_i64_SignedConversion_ReturnsTrue

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_conversion_test.cc`
**Start line**: 185
**Parameterized**: No

### Coverage
- Lines: 6.7% (2453/36711)
- Functions: 9.6% (214/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_f32_convert_i64`
**Intended scenario**: Load a wasm module with f32.convert_i64 instructions. Use standard LLVM path (`disable_llvm_intrinsics = false`), with signed conversion. Options include `opt_level=1`, `bounds_checks=2`.
**Intended outcome**: `aot_compile_wasm` returns true for the standard signed conversion path.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_f32_convert_i64` in `aot_emit_conversion.c` (called 4 times)
- `call_llvm_float_experimental_constrained_intrinsic` in `aot_emit_conversion.c` (called 1 time)
- `aot_call_llvm_intrinsic_v` / `__call_llvm_intrinsic` (called 1 time each)

**Uncovered paths** (from coverage-summary.md): Same infrastructure failure paths. No intrinsic capability checks triggered since `disable_llvm_intrinsics=false`.

**Actual code path**: The test compiles with the standard LLVM path for f32.convert_i64 signed conversion. The `call_llvm_float_experimental_constrained_intrinsic` is still called (this is the constrained intrinsic for float conversion accuracy, not the "disabled" intrinsic path). Compilation succeeds.

**Path type** (from coverage): SUCCESS

### Alignment: YES

The test name claims f32_convert_i64 signed conversion returning true, and coverage confirms `aot_compile_op_f32_convert_i64` is called through the standard path with successful compilation.

### Quality Screening

None

---

## Test Case [4/6]: EnhancedAotEmitConversionTest.aot_compile_op_f32_convert_i64_SignedWithIntrinsics_ReturnsTrue

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_conversion_test.cc`
**Start line**: 240
**Parameterized**: No

### Coverage
- Lines: 6.8% (2480/36711)
- Functions: 9.7% (217/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_f32_convert_i64`
**Intended scenario**: Load a wasm module with f32.convert_i64 instructions. Set `disable_llvm_intrinsics = true` to force the intrinsic path. Uses `opt_level=2`, `size_level=2`, `enable_simd=true`.
**Intended outcome**: `aot_compile_wasm` returns true through the intrinsic path.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_f32_convert_i64` in `aot_emit_conversion.c` (called 4 times)
- `aot_intrinsic_fill_capability_flags` in `aot_intrinsic.c` (called 1 time)
- `add_intrinsic_capability` in `aot_intrinsic.c` (called 2 times)
- `aot_intrinsic_check_capability` in `aot_intrinsic.c` (called 6 times)
- `call_llvm_float_experimental_constrained_intrinsic` (called 1 time)
- `aot_call_llvm_intrinsic_v` / `__call_llvm_intrinsic` (called 1 time each)

**Uncovered paths** (from coverage-summary.md): Same infrastructure failure paths.

**Actual code path**: With `disable_llvm_intrinsics=true`, the intrinsic capability flags are filled and checked. The compilation succeeds via the intrinsic-aware path for f32.convert_i64.

**Path type** (from coverage): SUCCESS

### Alignment: YES

The test name claims f32_convert_i64 signed with intrinsics returning true, and coverage confirms `aot_compile_op_f32_convert_i64` is called with intrinsic capability functions exercised.

### Quality Screening

None

---

## Test Case [5/6]: EnhancedAotEmitConversionTest.aot_compile_op_f32_demote_f64_Multiple_Operations

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_conversion_test.cc`
**Start line**: 295
**Parameterized**: No

### Coverage
- Lines: 6.7% (2464/36711)
- Functions: 9.6% (215/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_f32_demote_f64`
**Intended scenario**: Load a wasm module with multiple f32.demote_f64 operations. Standard compilation options with `opt_level=2`, `bounds_checks=2`. Tests that multiple demote operations compile correctly.
**Intended outcome**: `aot_compile_wasm` returns true.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_f32_demote_f64` in `aot_emit_conversion.c` (called 3 times)
- `call_llvm_float_experimental_constrained_intrinsic` (called 1 time)
- `aot_call_llvm_intrinsic_v` / `__call_llvm_intrinsic` (called 1 time each)

**Uncovered paths** (from coverage-summary.md): Same infrastructure failure paths.

**Actual code path**: The test compiles a wasm module with 3 f32.demote_f64 operations through the standard LLVM path. All operations compile successfully.

**Path type** (from coverage): SUCCESS

### Alignment: YES

The test name claims multiple f32_demote_f64 operations, and coverage confirms `aot_compile_op_f32_demote_f64` is called 3 times in a successful compilation.

### Quality Screening

None

---

## Test Case [6/6]: EnhancedAotEmitConversionTest.aot_compile_op_f64_convert_i64_SignedStandardPath_ReturnsTrue

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_conversion_test.cc`
**Start line**: 356
**Parameterized**: No

### Coverage
- Lines: 6.4% (2367/36711)
- Functions: 9.2% (205/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_f64_convert_i64`
**Intended scenario**: Load a wasm module with f64.convert_s_i64 instructions. Set `disable_llvm_intrinsics = true` and `opt_level=3`. The comment says "Force standard path" but `disable_llvm_intrinsics=true` actually forces the non-LLVM-intrinsic (constrained) path.
**Intended outcome**: `aot_compile_wasm` returns true.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_f64_convert_i64` in `aot_emit_conversion.c` (called 1 time)

**Uncovered paths** (from coverage-summary.md): Same infrastructure failure paths.

**Actual code path**: The test compiles a wasm module with f64.convert_s_i64 instruction. With `disable_llvm_intrinsics=true`, the constrained intrinsic path is used. Compilation succeeds.

**Path type** (from coverage): SUCCESS

### Alignment: NO

The test name claims "StandardPath" but `disable_llvm_intrinsics = true` forces the non-standard constrained intrinsic path. The comment "Force standard path" is also contradictory with the option value.

### Quality Screening

None

### Recommendations

**Issue**: Test name says "StandardPath" but `disable_llvm_intrinsics = true` forces the constrained intrinsic path, not the standard LLVM path.
**Fix**: Either rename to `aot_compile_op_f64_convert_i64_SignedConstrainedIntrinsicPath_ReturnsTrue` or change `disable_llvm_intrinsics` to `false` to actually test the standard path.

---

# Path Coverage Summary: enhanced_aot_emit_conversion_test.cc

## Function Coverage Analysis

| Target Function | SUCCESS | FAILURE | EDGE | Total | Status |
|-----------------|---------|---------|------|-------|--------|
| `aot_compile_op_i64_trunc_f32` | 1 | 0 | 0 | 1 | Missing FAILURE, EDGE |
| `aot_compile_op_f32_convert_i32` | 1 | 0 | 0 | 1 | Missing FAILURE, EDGE |
| `aot_compile_op_f32_convert_i64` | 2 | 0 | 0 | 2 | Missing FAILURE, EDGE |
| `aot_compile_op_f32_demote_f64` | 1 | 0 | 0 | 1 | Missing FAILURE, EDGE |
| `aot_compile_op_f64_convert_i64` | 1 | 0 | 0 | 1 | Missing FAILURE, EDGE |
| `aot_create_comp_data` | 5 | 0 | 0 | 5 | Missing FAILURE, EDGE |
| `aot_create_comp_context` | 5 | 0 | 0 | 5 | Missing FAILURE, EDGE |
| `aot_compile_wasm` | 6 | 0 | 0 | 6 | Missing FAILURE, EDGE |

**Status Criteria (STRICT):**
- No function has all 3 path types covered
- All functions only have SUCCESS path tests

## Enhancement Recommendations

> **Note**: These recommendations are preserved for future reference. The fix agent will **NOT** implement them at this time -- there may already be other test files covering the same paths. They serve as a backlog for future coverage improvements.

### `aot_compile_op_i64_trunc_f32` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_compile_op_i64_trunc_f32_LLVMBuildFPToSIFails_ReturnsFalse`
   - Scenario: Mock LLVMBuildFPToSI to return NULL, triggering the failure path in trunc_float_to_int
   - Expected: `aot_compile_wasm` returns false

2. `aot_compile_op_i64_trunc_f32_NullCompCtx_HandleGracefully`
   - Scenario: Pass NULL or invalid compilation context
   - Expected: Graceful failure handling

### `aot_compile_op_f32_convert_i32` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_compile_op_f32_convert_i32_LLVMBuildError_ReturnsFalse`
   - Scenario: Cause LLVM build instruction to fail during f32.convert_i32 emission
   - Expected: Compilation fails gracefully

### `aot_compile_op_f32_convert_i64` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_compile_op_f32_convert_i64_InvalidModule_ReturnsFalse`
   - Scenario: Provide a malformed module that causes conversion emission to fail
   - Expected: `aot_compile_wasm` returns false

### `aot_compile_op_f32_demote_f64` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_compile_op_f32_demote_f64_IntrinsicCallFails_ReturnsFalse`
   - Scenario: Force constrained intrinsic call to fail
   - Expected: Compilation fails gracefully

### `aot_compile_op_f64_convert_i64` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_compile_op_f64_convert_i64_BuildError_ReturnsFalse`
   - Scenario: Cause LLVM IR build failure during f64.convert_i64 emission
   - Expected: Compilation fails gracefully

### `aot_create_comp_data` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_create_comp_data_MallocFails_ReturnsNull`
   - Scenario: Trigger memory allocation failure (line 854-856)
   - Expected: Returns NULL with error message set

### `aot_create_comp_context` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_create_comp_context_AllocFails_ReturnsNull`
   - Scenario: Trigger allocation failure at context creation (line 2650-2652)
   - Expected: Returns NULL

### `aot_compile_wasm` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_compile_wasm_ValidationFails_ReturnsFalse`
   - Scenario: Provide invalid compilation data that fails validation (line 4026-4027)
   - Expected: Returns false

---

# Quality Issues Summary: enhanced_aot_emit_conversion_test.cc

| Test Case | Issue | Recommendation |
|-----------|-------|----------------|
| `aot_compile_op_f64_convert_i64_SignedStandardPath_ReturnsTrue` | Misleading name: "StandardPath" but uses `disable_llvm_intrinsics=true` | Rename test or fix option value |

**Total**: 1 issue found

---

# Static Analysis: enhanced_aot_emit_conversion_test.cc

## clang-tidy Results

| Line | Category | Message |
|------|----------|---------|
| 14 | cert-err58-cpp | initialization of `MAIN_WASM` with static storage duration may throw an exception that cannot be caught |
| 15 | cert-err58-cpp | initialization of `CONVERSION_WASM` with static storage duration may throw an exception that cannot be caught |
| 29 | modernize-use-nullptr | use `nullptr` instead of `NULL` |
| 33 | modernize-return-braced-init-list | avoid repeating the return type; use a braced initializer list |
| 88 | modernize-use-bool-literals | converting integer literal to bool, use bool literal instead (`AOTCompOption = {0}`) |
| 108 | modernize-use-nullptr | use `nullptr` instead of `NULL` in `aot_create_comp_data` call |
| 142 | modernize-use-bool-literals | converting integer literal to bool (`AOTCompOption = { 0 }`) |
| 154 | modernize-use-nullptr | use `nullptr` instead of `NULL` in `aot_create_comp_data` call |
| 193 | modernize-use-bool-literals | converting integer literal to bool (`AOTCompOption = {0}`) |
| 213 | modernize-use-nullptr | use `nullptr` instead of `NULL` in `aot_create_comp_data` call |
| 248 | modernize-use-bool-literals | converting integer literal to bool (`AOTCompOption = {0}`) |
| 268 | modernize-use-nullptr | use `nullptr` instead of `NULL` in `aot_create_comp_data` call |
| 303 | modernize-use-bool-literals | converting integer literal to bool (`AOTCompOption = {0}`) |
| 363 | modernize-use-bool-literals | converting integer literal to bool (`AOTCompOption = {0}`) |

**Summary**: 20 warnings treated as errors, 0 actual compilation errors
