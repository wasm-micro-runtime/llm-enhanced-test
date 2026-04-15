# Test Review Summary: enhanced_aot_emit_function_test.cc

## Redundancy Cleanup (from check_redundant_tests.js)

- **Original tests:** 26
- **Identified (redundant):** 17
- **Remaining tests (useful):** 9

### Redundant Test Cases (deleted in PHASE 1.5)
| Test Case | Reason | Status |
|-----------|--------|--------|
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_ValidTypeIndex_ProcessesSuccessfully` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_WithGcEnabled_ExercisesGcPath` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_TableIndexProcessing_ExercisesTableAccess` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_BasicBlockCreation_HandlesExceptionEmit` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_TableElemBasePointer_PerformsOffsetCalculation` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_GCEnabled_ProcessesFunctionObjects` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_GCDisabled_SkipsGCSpecificCode` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_GCEnabled_ValidTableElem_SuccessPath` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_GCEnabled_NullTableElem_ExceptionHandling` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_ref_null_TechnicalLimitation_Documented` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_ref_is_null_WithoutGC_ReturnsTrue` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_ref_is_null_NonGCEmptyStack_ReturnsFalse` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_MultipleReturnValues_ExceedsLimit_ReturnsFalse` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_MultipleReturnValues_LLVMConstFail_ReturnsFalse` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_call_MultipleReturnValues_LLVMBitCastFail_ReturnsFalse` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_ref_null_WithGC_PushesGCRef` | No incremental coverage contribution | Deleted |
| `EnhancedAotEmitFunctionTest.aot_compile_op_ref_null_WithoutGC_PushesI32Ref` | No incremental coverage contribution | Deleted |

---

## Test Case [1/9]: EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_InvalidTypeIndex_ReturnsFailure

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 164
**Parameterized**: No

### Coverage
- Lines: 7.9% (2893/36711)
- Functions: 10.4% (231/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_call_indirect`
**Intended scenario**: Pass an invalid type index (greater than `type_count`) to trigger the early validation failure at lines 2114-2116
**Intended outcome**: Function returns `false` due to invalid type index

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_call_indirect` in `aot_emit_function.c` (2 calls, 145 lines covered)
- `aot_compile_wasm` in `aot_compiler.c` (1 call, 14 lines covered)

**Uncovered paths** (from coverage-summary.md): Many LLVM build failure branches (`LLVMBuildGEP`, `LLVMBuildBitCast`, `LLVMBuildLoad` etc.), GC-specific paths, thread manager suspend check

**Actual code path**: The test first calls `aot_compile_wasm` (which succeeds, exercising the full compilation path for func_ctxes[0]), then calls `aot_compile_op_call_indirect` with an invalid type index. The first `aot_compile_wasm` call covers the SUCCESS path extensively (145 lines), while the second call with invalid type index hits the FAILURE early-return path at line 2114-2116.

**Path type** (from coverage): FAILURE (invalid type index causes early return false) + SUCCESS (via aot_compile_wasm)

### Alignment: YES

The test name says "InvalidTypeIndex_ReturnsFailure" and the test does indeed trigger the failure path by providing an out-of-range type index, with `ASSERT_FALSE(result)` confirming the failure return.

### Quality Screening

None

---

## Test Case [2/9]: EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_WithThreadMgr_ExercisesSuspendCheck

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 199
**Parameterized**: No

### Coverage
- Lines: 6.6% (2406/36711)
- Functions: 9.2% (204/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_call_indirect`
**Intended scenario**: Create a compilation context with thread manager enabled (`createCompContextWithOptions(module, false, true)`) to exercise the `check_suspend_flags` path at lines 2154-2156
**Intended outcome**: The suspend check insertion code path is exercised (no assertion on result)

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_call_indirect` in `aot_emit_function.c` (1 call, 26 lines covered)
- `check_suspend_flags` in `aot_emit_function.c` (1 call)

**Uncovered paths** (from coverage-summary.md): `check_suspend_flags` failure branch (line 2156), many LLVM build branches, GC paths, aot_frame commit paths

**Actual code path**: The function enters `aot_compile_op_call_indirect`, reaches the `enable_thread_mgr` check (line 2154), calls `check_suspend_flags` successfully, then proceeds further into the function. Only 26 lines covered suggests it proceeds into table access but fails at some LLVM operation.

**Path type** (from coverage): SUCCESS (for suspend check path specifically)

### Alignment: YES

The test name says "WithThreadMgr_ExercisesSuspendCheck" and coverage confirms `check_suspend_flags` was called 1 time with thread manager enabled.

### Quality Screening

- Missing assertions: `result` variable is stored but never asserted (dead store confirmed by clang-tidy)

### Recommendations

**Issue**: The `result` variable is never used or asserted, making this test unable to detect regressions
**Fix**: Add `ASSERT_TRUE(result)` or `ASSERT_FALSE(result)` depending on expected behavior, or at minimum verify the return value is checked

---

## Test Case [3/9]: EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_GCEnabled_PointerSizeOffset_ValidAccess

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 234
**Parameterized**: No

### Coverage
- Lines: 6.9% (2526/36711)
- Functions: 9.4% (209/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_call_indirect`
**Intended scenario**: Enable GC and push an I32 value onto the stack, then call `aot_compile_op_call_indirect` to exercise pointer-size-based offset calculations at lines 2308-2331
**Intended outcome**: GC path processing exercises pointer arithmetic (test accepts either true or false result)

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_call_indirect` in `aot_emit_function.c` (1 call, 76 lines covered)
- `aot_value_stack_push` in `aot_llvm.c` (1 call)

**Uncovered paths** (from coverage-summary.md): Thread manager suspend check, many LLVM failure branches, non-GC paths

**Actual code path**: With GC enabled, the function enters the GC-specific code path in `aot_compile_op_call_indirect`. The 76 lines covered (vs 26 without GC in test 2) confirm the GC path is being exercised, including table element access and pointer offset calculations.

**Path type** (from coverage): SUCCESS (GC-enabled compilation path)

### Alignment: NO

The test name says "PointerSizeOffset_ValidAccess" but the assertion is `ASSERT_TRUE(result == true || result == false)` which is a tautology that always passes regardless of outcome.

### Recommendations

**Issue**: Tautological assertion `ASSERT_TRUE(result == true || result == false)` provides no verification; test name implies valid access but does not actually verify success
**Fix**: Replace with `ASSERT_TRUE(result)` if the GC path should succeed, or remove the tautological assertion and add meaningful checks on the compilation output

---

## Test Case [4/9]: EnhancedAotEmitFunctionTest.aot_compile_op_call_indirect_GCEnabled_MultipleTableAccess_CompleteFlow

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 291
**Parameterized**: No

### Coverage
- Lines: 6.9% (2526/36711)
- Functions: 9.4% (209/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_call_indirect`
**Intended scenario**: Enable GC and ref_types, push an I32 value onto the stack, call `aot_compile_op_call_indirect` to exercise the "complete flow" of multiple table access with GC
**Intended outcome**: Complete flow is exercised (test accepts either true or false result)

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_call_indirect` in `aot_emit_function.c` (1 call, 76 lines covered)
- `aot_value_stack_push` in `aot_llvm.c` (1 call)

**Uncovered paths** (from coverage-summary.md): Same as test 3 — identical coverage profile (76 lines in target function)

**Actual code path**: Identical to test 3. Same GC-enabled path, same 76 lines covered, same functions called. The "multiple table access" and "complete flow" described in the test name are not reflected in different coverage.

**Path type** (from coverage): SUCCESS (GC-enabled compilation path)

### Alignment: NO

The test name says "MultipleTableAccess_CompleteFlow" but the test only calls `aot_compile_op_call_indirect` once with `tbl_idx=0`, which is the same single table access as test 3. The assertion `ASSERT_TRUE(result1 == true || result1 == false)` is a tautology.

### Recommendations

**Issue**: Test name claims "multiple table access" and "complete flow" but coverage is identical to test 3 with a single call to table index 0; tautological assertion provides no verification
**Fix**: Either test with multiple different table indices to justify the name, or rename to accurately reflect what is tested. Replace the tautological assertion with a meaningful one.

---

## Test Case [5/9]: EnhancedAotEmitFunctionTest.aot_compile_op_ref_is_null_WithGCEnabled_ReturnsTrue

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 348
**Parameterized**: No

### Coverage
- Lines: 6.5% (2388/36711)
- Functions: 9.1% (203/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_ref_is_null`
**Intended scenario**: Enable GC, push a `VALUE_TYPE_GC_REF` null pointer onto the value stack, then call `aot_compile_op_ref_is_null` to exercise the GC-enabled path
**Intended outcome**: Function returns `true` (success), and the result on the stack is `VALUE_TYPE_I32`

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_ref_is_null` in `aot_emit_function.c` (1 call, 8 lines covered)

**Uncovered paths** (from coverage-summary.md): `LLVMBuildIsNull` failure branch (lines 2772-2773), non-GC path (`POP_I32` at line 2777), `LLVMBuildZExt` failure (lines 2787-2788)

**Actual code path**: The GC-enabled path is taken: `POP_GC_REF` pops the value, `LLVMBuildIsNull` creates the null check, `LLVMBuildZExt` extends result to I32, and `PUSH_I32` pushes the result. This is the SUCCESS path through the GC branch.

**Path type** (from coverage): SUCCESS

### Alignment: YES

The test name says "WithGCEnabled_ReturnsTrue" and the function does return true via the GC path, verified by `ASSERT_TRUE(result)`.

### Quality Screening

None

---

## Test Case [6/9]: EnhancedAotEmitFunctionTest.aot_compile_op_ref_is_null_EmptyStack_ReturnsFalse

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 403
**Parameterized**: No

### Coverage
- Lines: 6.5% (2372/36711)
- Functions: 9.0% (201/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_ref_is_null`
**Intended scenario**: Enable GC but set the value stack to empty (null head/end), then call `aot_compile_op_ref_is_null` to trigger a stack underflow error
**Intended outcome**: Function returns `false` due to empty stack

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_ref_is_null` in `aot_emit_function.c` (1 call, 6 lines covered)

**Uncovered paths** (from coverage-summary.md): `LLVMBuildIsNull` (line 2771) and subsequent lines not reached; non-GC path (lines 2777-2782); `LLVMBuildZExt`/`PUSH_I32` success path (lines 2791-2793)

**Actual code path**: The function enters the GC-enabled branch, attempts `POP_GC_REF` which fails because the stack is empty, and jumps to the `fail` label, returning false. Only 6 lines covered (entry + GC branch check + POP attempt + fail path).

**Path type** (from coverage): FAILURE

### Alignment: YES

The test name says "EmptyStack_ReturnsFalse" and the test correctly triggers the stack underflow failure path, verified by `ASSERT_FALSE(result)`.

### Quality Screening

None

---

## Test Case [7/9]: EnhancedAotEmitFunctionTest.aot_compile_op_call_MultipleReturnValues_WithinLimit_ReturnsTrue

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 443
**Parameterized**: No

### Coverage
- Lines: 6.7% (2474/36711)
- Functions: 9.2% (205/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_call`
**Intended scenario**: Create a function type with 3 I32 return values (`result_count = 3`) to trigger the multiple return values code path (lines 1511-1546), then call `aot_compile_op_call`
**Intended outcome**: The multiple return value code path is exercised (test accepts either true or false result)

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_call` in `aot_emit_function.c` (1 call, 75 lines covered)

**Uncovered paths** (from coverage-summary.md): Many LLVM build failure branches, GC-specific call paths, tail call paths, import function paths

**Actual code path**: The function enters `aot_compile_op_call`, processes the function type with multiple return values, and exercises 75 lines of the compilation logic including multi-return handling.

**Path type** (from coverage): SUCCESS

### Alignment: NO

The test name says "WithinLimit_ReturnsTrue" but the assertion is `ASSERT_TRUE(result == true || result == false)` which is a tautology. The test does not verify the function returns true.

### Recommendations

**Issue**: Tautological assertion `ASSERT_TRUE(result == true || result == false)` does not verify the "ReturnsTrue" claim in the test name
**Fix**: Replace with `ASSERT_TRUE(result)` to match the test name's claim, or rename the test if failure is expected

---

## Test Case [8/9]: EnhancedAotEmitFunctionTest.aot_compile_op_call_MultipleReturnValues_LLVMGEPFail_ReturnsFalse

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 529
**Parameterized**: No

### Coverage
- Lines: 6.7% (2451/36711)
- Functions: 9.2% (205/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_call`
**Intended scenario**: Call `aot_compile_op_call` to exercise the GEP (GetElementPtr) operation code path, expecting the GEP failure handling path to be covered
**Intended outcome**: If failure occurs, verify error message is set; accepts either true or false result

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_call` in `aot_emit_function.c` (1 call, 55 lines covered)

**Uncovered paths** (from coverage-summary.md): Same set of LLVM build failure branches, GC paths, tail call paths

**Actual code path**: The function enters `aot_compile_op_call` with a simple single-return function type (no multi-return type modification like test 7). Covers 55 lines (less than test 7's 75 lines because the multi-return path is not triggered). This test does NOT actually trigger the GEP failure path — it just exercises the normal compilation with a single-return function.

**Path type** (from coverage): SUCCESS

### Alignment: NO

The test name says "LLVMGEPFail_ReturnsFalse" implying a GEP failure path should be triggered, but the test has no mechanism to force LLVM GEP operations to fail. The actual path covered is the SUCCESS path with 55 lines. The assertion `ASSERT_TRUE(result == true || result == false)` is a tautology that contradicts the "ReturnsFalse" in the name.

### Recommendations

**Issue**: Test name claims "LLVMGEPFail_ReturnsFalse" but there is no way to force LLVM GEP to fail in this test. The actual code path is the normal SUCCESS path, not a failure path. The tautological assertion cannot verify failure.
**Fix**: Either mock/stub LLVM GEP to return null (if possible in the test framework), or rename the test to accurately reflect it tests the normal `aot_compile_op_call` SUCCESS path

---

## Test Case [9/9]: EnhancedAotEmitFunctionTest.aot_compile_op_ref_null_CompleteCoverage_ExercisesAllLines

**File**: `llm-enhanced-testcompilation-emit/enhanced_aot_emit_function_test.cc`
**Start line**: 573
**Parameterized**: No

### Coverage
- Lines: 6.5% (2376/36711)
- Functions: 9.0% (201/2228)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `aot_compile_op_ref_null`
**Intended scenario**: Test both GC-enabled and GC-disabled paths by creating two compilation contexts. Part 1 calls with GC enabled (lines 2751, 2753 true, 2754, 2758). Part 2 calls with GC disabled but ref_types enabled (lines 2751, 2753 false, 2756, 2758).
**Intended outcome**: Both paths return `true`, covering all lines 2751-2758

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `aot_compile_op_ref_null` in `aot_emit_function.c` (2 calls, 5 lines covered out of 7)

**Uncovered paths** (from coverage-summary.md): `fail` label at lines 2759-2760 (only reachable if PUSH operations fail internally)

**Actual code path**: Both GC and non-GC branches of `aot_compile_op_ref_null` are covered. The GC path pushes a GC_REF null, the non-GC path pushes an I32 null. Both return true. Only the fail path (lines 2759-2760) remains uncovered, which requires internal PUSH operation failure.

**Path type** (from coverage): SUCCESS

### Alignment: YES

The test name says "CompleteCoverage_ExercisesAllLines" and coverage shows 5/7 lines covered (the only uncovered lines are the fail label which is extremely hard to trigger). Both GC and non-GC paths are exercised with `ASSERT_TRUE` verification.

### Quality Screening

None

---

# Path Coverage Summary: enhanced_aot_emit_function_test.cc

## Function Coverage Analysis

| Target Function | SUCCESS | FAILURE | EDGE | Total | Status |
|-----------------|---------|---------|------|-------|--------|
| `aot_compile_op_call_indirect` | 3 (tests 2,3,4) | 1 (test 1) | 0 | 4 | Missing EDGE |
| `aot_compile_op_ref_is_null` | 1 (test 5) | 1 (test 6) | 0 | 2 | Missing EDGE |
| `aot_compile_op_call` | 2 (tests 7,8) | 0 | 0 | 2 | Missing FAILURE, EDGE |
| `aot_compile_op_ref_null` | 1 (test 9) | 0 | 0 | 1 | Missing FAILURE, EDGE |

## Enhancement Recommendations

> **Note**: These recommendations are preserved for future reference. The fix agent will **NOT** implement them at this time -- there may already be other test files covering the same paths. They serve as a backlog for future coverage improvements.

### `aot_compile_op_call_indirect` - Missing EDGE path

**Suggested test cases**:
1. `aot_compile_op_call_indirect_ZeroTypeCount_HandlesEdgeCase`
   - Scenario: Set `type_count = 0` and pass `type_idx = 0` (boundary condition)
   - Expected: Returns false (type index out of range when count is 0)

2. `aot_compile_op_call_indirect_MaxTableIndex_BoundaryCheck`
   - Scenario: Use the maximum valid table index to exercise boundary table access code
   - Expected: Exercises boundary offset calculations

### `aot_compile_op_ref_is_null` - Missing EDGE path

**Suggested test cases**:
1. `aot_compile_op_ref_is_null_NonGCPath_WithI32Value`
   - Scenario: Disable GC, push an I32 value, call `aot_compile_op_ref_is_null` to exercise the non-GC `POP_I32` + `LLVMBuildICmp` path
   - Expected: Returns true, exercises lines 2777-2782

### `aot_compile_op_call` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_compile_op_call_InvalidFuncIndex_ReturnsFalse`
   - Scenario: Pass an out-of-range function index
   - Expected: Returns false with appropriate error

2. `aot_compile_op_call_ImportFunction_ExercisesImportPath`
   - Scenario: Call an imported function (func_idx < import_func_count) to exercise the import function compilation path
   - Expected: Exercises the import function code path

3. `aot_compile_op_call_TailCall_ExercisesTailCallPath`
   - Scenario: Set `tail_call=true` parameter
   - Expected: Exercises tail call optimization path

### `aot_compile_op_ref_null` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `aot_compile_op_ref_null_NullCompCtx_HandlesGracefully`
   - Scenario: Pass a comp_ctx with corrupted builder to trigger PUSH failure leading to fail label
   - Expected: Returns false (fail path at lines 2759-2760)

---

# Quality Issues Summary: enhanced_aot_emit_function_test.cc

| Test Case | Issue | Recommendation |
|-----------|-------|----------------|
| `aot_compile_op_call_indirect_WithThreadMgr_ExercisesSuspendCheck` | Dead store: `result` never asserted (confirmed by clang-tidy) | Add assertion on result |
| `aot_compile_op_call_indirect_GCEnabled_PointerSizeOffset_ValidAccess` | Tautological assertion `ASSERT_TRUE(result == true \|\| result == false)` | Replace with meaningful assertion |
| `aot_compile_op_call_indirect_GCEnabled_MultipleTableAccess_CompleteFlow` | Tautological assertion `ASSERT_TRUE(result1 == true \|\| result1 == false)` | Replace with meaningful assertion |
| `aot_compile_op_call_MultipleReturnValues_WithinLimit_ReturnsTrue` | Tautological assertion `ASSERT_TRUE(result == true \|\| result == false)` | Replace with `ASSERT_TRUE(result)` |
| `aot_compile_op_call_MultipleReturnValues_LLVMGEPFail_ReturnsFalse` | Tautological assertion; name claims failure but tests success path | Rename test and add meaningful assertion |

**Total**: 5 issues found

---

# Static Analysis: enhanced_aot_emit_function_test.cc

## clang-tidy Results

| Line | Category | Message |
|------|----------|---------|
| 11 | modernize-deprecated-headers | inclusion of deprecated C++ header 'limits.h'; consider using 'climits' |
| 66 | modernize-use-nullptr | use nullptr instead of NULL |
| 67 | readability-implicit-bool-conversion | implicit conversion 'AOTCompData *' -> bool |
| 70 | modernize-use-bool-literals | converting integer literal to bool |
| 70 | readability-implicit-bool-conversion | implicit conversion 'int' -> bool |
| 88 | readability-implicit-bool-conversion | implicit conversion 'AOTCompContext *' -> bool |
| 97 | readability-implicit-bool-conversion | implicit conversion 'AOTCompContext *' -> bool |
| 99 | modernize-use-auto | use auto when initializing with a cast |
| 101 | readability-implicit-bool-conversion | implicit conversion 'AOTCompData *' -> bool |
| 107 | readability-implicit-bool-conversion | implicit conversion 'AOTBlock *' -> bool |
| 113 | modernize-use-auto | use auto when initializing with a cast |
| 114 | readability-implicit-bool-conversion | implicit conversion 'AOTValue *' -> bool |
| 117 | readability-implicit-bool-conversion | implicit conversion bool -> 'LLVMBool' (aka 'int') |
| 134 | modernize-use-auto | use auto when initializing with a cast |
| 138 | readability-implicit-bool-conversion | implicit conversion bool -> 'LLVMBool' (aka 'int') |
| 142 | modernize-use-auto | use auto when initializing with a cast |
| 146 | readability-implicit-bool-conversion | implicit conversion bool -> 'LLVMBool' (aka 'int') |
| 216 | clang-analyzer-deadcode.DeadStores | Value stored to 'result' during its initialization is never read |
| 252 | modernize-use-auto | use auto when initializing with a cast |
| 256 | readability-implicit-bool-conversion | implicit conversion bool -> 'LLVMBool' (aka 'int') |
| 258 | readability-implicit-bool-conversion | implicit conversion 'AOTBlock *' -> bool |
| 306 | modernize-use-auto | use auto when initializing with a cast |
| 310 | readability-implicit-bool-conversion | implicit conversion bool -> 'LLVMBool' (aka 'int') |
| 312 | readability-implicit-bool-conversion | implicit conversion 'AOTBlock *' -> bool |
| 362 | modernize-use-auto | use auto when initializing with a cast |
| 367 | readability-implicit-bool-conversion | implicit conversion 'AOTBlock *' -> bool |
| 379 | readability-implicit-bool-conversion | implicit conversion 'AOTBlock *' -> bool |
| 416 | readability-implicit-bool-conversion | implicit conversion 'AOTBlock *' -> bool |
| 451 | modernize-use-auto | use auto when initializing with a cast |
| 470 | modernize-use-auto | use auto when initializing with a cast |

**Summary**: 30 warnings treated as errors, 0 actual errors
