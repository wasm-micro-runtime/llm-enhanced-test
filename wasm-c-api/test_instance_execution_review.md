# Test Review Summary: test_instance_execution.cc

## Redundancy Cleanup (from check_redundant_tests.js)

- **Original tests:** 21
- **Identified (redundant):** 17
- **Remaining tests (useful):** 4

### Redundant Test Cases (deleted in PHASE 1.5)
| Test Case | Reason | Status |
|-----------|--------|--------|
| `InstanceExecutionTest.Instance_CreateFromValidModule_SucceedsCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_CreateWithNullStore_ReturnsNull` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_CreateWithNullImports_ReturnsNull` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_Delete_HandlesNullGracefully` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_GetExports_ReturnsCorrectCount` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Trap_GetMessage_ReturnsValidMessage` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Trap_GetTrace_HandlesCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Frame_Operations_HandleNullGracefully` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_MultipleCreation_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_ResourceCleanup_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_StoreIntegration_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_CompleteLifecycle_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_TypeValidation_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_ModuleAssociation_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_MemoryManagement_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_ErrorRecovery_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |
| `InstanceExecutionTest.Instance_ConcurrentAccess_WorksCorrectly` | No incremental coverage contribution | ✅ Deleted |

---

## Test Case [1/4]: InstanceExecutionTest.Instance_CreateWithNullModule_ReturnsNull

**File**: `smart-tests/wasm-c-api/test_instance_execution.cc`
**Start line**: 77
**Parameterized**: No

### Coverage
- Lines: 4.3% (673/15824)
- Functions: 8.5% (110/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_instance_new`
**Intended scenario**: Call `wasm_instance_new` with a NULL module pointer; the test name "ReturnsNull" implies the function should reject a null module and return NULL.
**Intended outcome**: `wasm_instance_new` returns NULL; assertion `ASSERT_EQ(nullptr, null_instance)` passes.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA, guided by Expected Purpose above):
- `wasm_instance_new` in `wasm_c_api.c` (delegates immediately)
- `wasm_instance_new_with_args` in `wasm_c_api.c` (thin wrapper)
- `wasm_instance_new_with_args_ex` in `wasm_c_api.c` (actual null-module check at line 4961)

**Uncovered paths** (from coverage-summary.md): Lines 4970+ in `wasm_instance_new_with_args_ex` — malloc, do_link, full instantiation path — are all uncovered because execution returns NULL at line 4962 immediately after the `!module` check at line 4961.

**Actual code path**: `wasm_instance_new` → `wasm_instance_new_with_args` → `wasm_instance_new_with_args_ex`; null check `if (!module)` at line 4961 evaluates true; returns NULL at line 4962. No allocation or instantiation occurs.

**Path type** (from coverage): FAILURE

### Alignment: YES

The test name "ReturnsNull" correctly describes the null-module early-return failure path that coverage confirms is exercised in `wasm_instance_new_with_args_ex`.

### Quality Screening

None.

---

## Test Case [2/4]: InstanceExecutionTest.Instance_GetExportsFromNull_HandlesGracefully

**File**: `smart-tests/wasm-c-api/test_instance_execution.cc`
**Start line**: 87
**Parameterized**: No

### Coverage
- Lines: 4.2% (659/15824)
- Functions: 8.4% (108/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_instance_exports`
**Intended scenario**: Call `wasm_instance_exports` with a NULL instance pointer; the test name "HandlesGracefully" implies the function should tolerate a null instance without crashing and leave the output vector empty.
**Intended outcome**: After the call, `exports.size == 0` and `exports.data == nullptr`.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA, guided by Expected Purpose above):
- `wasm_instance_exports` in `wasm_c_api.c` (null guard at line 5194)

**Uncovered paths** (from coverage-summary.md): Line 5197 (`wasm_extern_vec_copy`) is NOT covered — the path where instance is valid and exports are actually copied is never reached.

**Actual code path**: `wasm_instance_exports` is called with `nullptr`; the guard `if (!instance || !out)` at line 5194 evaluates true; function returns at line 5195 without doing anything. The exports vector remains empty as initialized by `wasm_extern_vec_new_empty`.

**Path type** (from coverage): FAILURE

### Alignment: YES

The test name "HandlesGracefully" aligns with the null-instance early-return path confirmed by coverage.

### Quality Screening

None.

---

## Test Case [3/4]: InstanceExecutionTest.Trap_GetOrigin_ReturnsValidFrame

**File**: `smart-tests/wasm-c-api/test_instance_execution.cc`
**Start line**: 98
**Parameterized**: No

### Coverage
- Lines: 4.2% (668/15824)
- Functions: 8.2% (106/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_trap_origin`
**Intended scenario**: Create a trap manually using `wasm_trap_new` with a message string, then call `wasm_trap_origin` and retrieve the origin frame. The test name "ReturnsValidFrame" implies the function should return a usable frame pointer.
**Intended outcome**: `wasm_trap_new` succeeds (ASSERT_NE nullptr); `wasm_trap_origin` returns a frame (possibly NULL for manually-created traps, which the test conditionally handles).

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA, guided by Expected Purpose above):
- `wasm_trap_new` in `wasm_c_api.c` (SUCCESS path: allocates trap, copies message)
- `wasm_trap_origin` in `wasm_c_api.c` (FAILURE/EDGE path: null frames check)

**Uncovered paths** (from coverage-summary.md): Lines 2074-2075 in `wasm_trap_origin` — the path where `trap->frames` is non-null and a frame is copied — is NOT covered. For a manually-created trap without a runtime execution context, `trap->frames` is NULL, so `wasm_trap_origin` always returns NULL in this scenario.

**Actual code path**: `wasm_trap_new` creates a trap with a message (SUCCESS path covered). `wasm_trap_origin` checks `!trap->frames || !trap->frames->num_elems` at line 2069 — evaluates true (manually-created trap has no frames) — returns NULL at line 2070. The test's conditional `if (frame)` branch is never entered.

**Path type** (from coverage): SUCCESS (for `wasm_trap_new`) + EDGE (for `wasm_trap_origin` — null frames guard)

### Alignment: NO

The test name "ReturnsValidFrame" implies `wasm_trap_origin` returns a non-null frame, but coverage shows only the null-return path (line 2070) is exercised; line 2075 (`wasm_frame_copy`) is never reached. A manually-created trap has no execution frames, so the stated outcome is never actually achieved.

### Recommendations

**Issue**: The test name claims "ReturnsValidFrame" but `wasm_trap_origin` always returns NULL for manually-created traps. The function path that actually returns a frame (lines 2074-2075) is not covered.
**Fix**: Rename the test to `Trap_GetOrigin_ReturnsNullForManualTrap_HandlesCorrectly` to reflect the real behavior, and update the assertion to `ASSERT_EQ(nullptr, frame)` rather than relying on the conditional branch. Alternatively, create a separate test that exercises a trap produced during real WASM execution to cover the non-null frame path.

---

## Test Case [4/4]: InstanceExecutionTest.Instance_WithInvalidImports_HandlesGracefully

**File**: `smart-tests/wasm-c-api/test_instance_execution.cc`
**Start line**: 120
**Parameterized**: No

### Coverage
- Lines: 4.7% (737/15824)
- Functions: 9.5% (122/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_instance_new` / `do_link`
**Intended scenario**: Create a `wasm_func_t` with a NULL callback (representing an "invalid" import), wrap it as an extern, and pass it to `wasm_instance_new`. The test name "HandlesGracefully" implies the runtime should not crash and should return NULL or succeed without errors.
**Intended outcome**: `wasm_instance_new` either returns NULL (graceful rejection) or a valid instance; the test handles both cases conditionally.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA, guided by Expected Purpose above):
- `wasm_func_new` in `wasm_c_api.c` (FAILURE path: NULL callback check at line 3088)
- `do_link` in `wasm_c_api.c` (FAILURE path: NULL import element check at lines 4849-4851)
- `wasm_instance_new_with_args_ex` in `wasm_c_api.c` (FAILURE path via do_link returning false)

**Uncovered paths** (from coverage-summary.md): Lines 4854-4918 in `do_link` (the entire switch statement handling EXTERN_FUNC, EXTERN_GLOBAL, etc.) are NOT covered because execution fails at the NULL import check before reaching the switch. Lines 3091+ in `wasm_func_new` (the successful function creation path) are NOT covered.

**Actual code path**: `wasm_func_new(store, func_type, nullptr)` returns NULL at line 3089 because the callback is NULL. `wasm_func_as_extern(NULL)` returns NULL/invalid extern. `do_link` iterates the imports array, finds `imports->data[0]` is NULL (line 4847 covered, line 4849 null check true), logs error at line 4850, and jumps to `failed` returning false. `wasm_instance_new_with_args_ex` then follows the failure path and returns NULL.

**Path type** (from coverage): FAILURE

### Alignment: YES

The test name "HandlesGracefully" aligns with the failure path confirmed by coverage — the runtime rejects the NULL import gracefully without crashing.

### Quality Screening

- Missing assertions: The test uses a conditional `if (null_instance)` without an unconditional assertion about the expected outcome. Given that `wasm_func_new(store, func_type, nullptr)` always returns NULL (coverage confirms this), `null_instance` is always NULL, but the test does not assert this with `ASSERT_EQ(nullptr, null_instance)`. The test merely avoids crashing rather than verifying a specific behavior.

---

# Path Coverage Summary: test_instance_execution.cc

## Function Coverage Analysis

| Target Function | SUCCESS | FAILURE | EDGE | Total | Status |
|-----------------|---------|---------|------|-------|--------|
| `wasm_instance_new` / `wasm_instance_new_with_args_ex` | 0 | 2 | 0 | 2 | ❌ Missing SUCCESS, EDGE |
| `wasm_instance_exports` | 0 | 1 | 0 | 1 | ❌ Missing SUCCESS, EDGE |
| `wasm_trap_new` | 1 | 0 | 0 | 1 | ❌ Missing FAILURE, EDGE |
| `wasm_trap_origin` | 0 | 0 | 1 | 1 | ❌ Missing SUCCESS, FAILURE |
| `do_link` | 0 | 1 | 0 | 1 | ❌ Missing SUCCESS, EDGE |
| `wasm_func_new` | 0 | 1 | 0 | 1 | ❌ Missing SUCCESS, EDGE |

**Status Criteria (STRICT):**
- ✅ **Complete**: Function has at least one test for EACH of SUCCESS, FAILURE, and EDGE paths
- ⚠️ **Missing X**: Function is missing one or more path types - MUST recommend new tests
- ❌ **Poor**: Function has only 1 path type covered - high priority for enhancement

## Enhancement Recommendations

> **Note**: These recommendations are preserved for future reference. The fix agent will **NOT** implement them at this time — there may already be other test files covering the same paths. They serve as a backlog for future coverage improvements.

### `wasm_instance_new` / `wasm_instance_new_with_args_ex` - Missing SUCCESS and EDGE paths

**Suggested test cases**:
1. `Instance_CreateFromValidModule_SuccessPath_CreatesInstance`
   - Scenario: Load a minimal valid WASM module, call `wasm_instance_new` with a valid store, module, and empty imports; assert the returned instance is non-null and delete it.
   - Expected: Returns a valid `wasm_instance_t*`, full instantiation path through `malloc_internal`, `do_link`, and `wasm_runtime_instantiate` is covered.
2. `Instance_CreateWithNullStore_EdgeCase_ReturnsNull`
   - Scenario: Call `wasm_instance_new` with a valid module but NULL store; verify null is returned without crash.
   - Expected: Returns NULL; edge guard at `bh_assert(singleton_engine)` or early null check is triggered.

### `wasm_instance_exports` - Missing SUCCESS and EDGE paths

**Suggested test cases**:
1. `Instance_GetExports_ValidInstance_ReturnsCopiedVec`
   - Scenario: Create a valid instance from a module that exports at least one function, then call `wasm_instance_exports` with a valid output vector.
   - Expected: `exports.size > 0`; the `wasm_extern_vec_copy` path at line 5197 is covered.
2. `Instance_GetExports_NullOut_HandlesGracefully`
   - Scenario: Call `wasm_instance_exports` with a valid instance but NULL `out` pointer.
   - Expected: Returns without crash; line 5194 null-out guard covered.

### `wasm_trap_new` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `Trap_New_NullStore_ReturnsNull`
   - Scenario: Call `wasm_trap_new(nullptr, &message)` with a valid message but NULL store.
   - Expected: Returns NULL; line 2023 is covered.
2. `Trap_New_NullMessage_CreatesEmptyTrap`
   - Scenario: Call `wasm_trap_new(store, nullptr)` with a valid store but NULL message.
   - Expected: Returns a non-null trap with an empty message; line 2030 false-branch is covered.

### `wasm_trap_origin` - Missing SUCCESS and FAILURE paths

**Suggested test cases**:
1. `Trap_GetOrigin_ExecutionTrap_ReturnsFrame`
   - Scenario: Execute a WASM function that causes a runtime trap (e.g., unreachable or divide-by-zero), capture the trap via the `wasm_instance_new` trap parameter, then call `wasm_trap_origin`.
   - Expected: Returns a non-null `wasm_frame_t*`; lines 2074-2075 are covered.
2. `Trap_GetOrigin_NullTrap_ReturnsNull`
   - Scenario: Call `wasm_trap_origin(nullptr)`.
   - Expected: Returns NULL; null trap guard at line 2069 covered.

### `do_link` - Missing SUCCESS and EDGE paths

**Suggested test cases**:
1. `Instance_LinkValidFunc_SuccessPath_Instantiates`
   - Scenario: Create a WASM module requiring one import function, provide a valid host function with the correct signature, call `wasm_instance_new`.
   - Expected: `do_link` succeeds; `interp_link_func` or `aot_link_func` path is covered.
2. `Instance_LinkEmptyImports_EdgeCase_Succeeds`
   - Scenario: Create a WASM module requiring zero imports, call `wasm_instance_new` with an empty imports vector.
   - Expected: `do_link` loop body is never entered (0 iterations); return true at line 4918 is covered.

### `wasm_func_new` - Missing SUCCESS and EDGE paths

**Suggested test cases**:
1. `Func_New_ValidCallback_CreatesFunc`
   - Scenario: Call `wasm_func_new` with a valid store, functype, and non-null callback.
   - Expected: Returns a non-null `wasm_func_t*`; `wasm_func_new_basic` path at line 3091 is covered.

---

# Quality Issues Summary: test_instance_execution.cc

| Test Case | Issue | Recommendation |
|-----------|-------|----------------|
| `InstanceExecutionTest.Instance_WithInvalidImports_HandlesGracefully` | Missing unconditional assertion — uses `if (null_instance)` conditional instead of asserting the expected NULL result | Add `ASSERT_EQ(nullptr, null_instance)` after the `wasm_instance_new` call to explicitly verify the expected failure outcome |

**Total**: 1 issue found

---

# Static Analysis: test_instance_execution.cc

## clang-tidy Results

| Line | Category | Message |
|------|----------|---------|
| 38 | readability-implicit-bool-conversion | implicit conversion `wasm_instance_t *` -> bool; use `!= nullptr` |
| 42 | readability-implicit-bool-conversion | implicit conversion `wasm_module_t *` -> bool; use `!= nullptr` |
| 46 | readability-implicit-bool-conversion | implicit conversion `wasm_store_t *` -> bool; use `!= nullptr` |
| 50 | readability-implicit-bool-conversion | implicit conversion `wasm_engine_t *` -> bool; use `!= nullptr` |
| 110 | readability-implicit-bool-conversion | implicit conversion `wasm_frame_t *` -> bool; use `!= nullptr` |
| 139 | readability-implicit-bool-conversion | implicit conversion `wasm_instance_t *` -> bool; use `!= nullptr` |

**Summary**: 6 warnings treated as errors (all readability-implicit-bool-conversion)
