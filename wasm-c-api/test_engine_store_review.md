# Test Review Summary: test_engine_store.cc

## Redundancy Cleanup (from check_redundant_tests.js)

- **Original tests:** 20
- **Identified (redundant):** 11
- **Remaining tests (useful):** 9

### Redundant Test Cases (deleted in PHASE 1.5)
| Test Case | Reason | Status |
|-----------|--------|--------|
| `EngineStoreTest.EngineConfig_Default_CreatesSuccessfully` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.EngineConfig_WithMemAllocator_CreatesSuccessfully` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.EngineConfig_NullConfig_HandlesGracefully` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.Engine_MultipleCalls_ReturnsSameInstance` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.Engine_AfterDelete_CanRecreate` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.Store_WithValidEngine_CreatesSuccessfully` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.Store_MultipleInstances_AreIsolated` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.Store_RecreateAfterDelete_WorksCorrectly` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.Engine_ConcurrentCreation_ThreadSafe` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.Store_DoubleDelete_HandlesGracefully` | No incremental coverage contribution | Deleted |
| `EngineStoreTest.Store_LargeNumberOfStores_HandlesCorrectly` | No incremental coverage contribution | Deleted |

---

## Test Case [1/9]: EngineStoreTest.EngineConfig_WithLinuxPerf_CreatesSuccessfully

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 33
**Parameterized**: No

### Coverage
- Lines: 2.3% (370/15824)
- Functions: 4.5% (58/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_config_set_linux_perf_opt` / `wasm_engine_new_with_config`
**Intended scenario**: Create a config with Linux perf profiling enabled, then create an engine from it; verifies the engine is successfully created.
**Intended outcome**: `wasm_config_set_linux_perf_opt` returns non-null, engine creation succeeds, `ASSERT_NE(nullptr, engine)`.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_config_set_linux_perf_opt` in `wasm_c_api.c` (called once)
- `wasm_engine_new_with_config` in `wasm_c_api.c` (called once)
- `wasm_engine_new_internal` in `wasm_c_api.c` (called once)
- `wasm_engine_delete` in `wasm_c_api.c` (called once)

**Uncovered paths**: Error paths inside `wasm_engine_new_internal` (malloc failures, `wasm_runtime_full_init` failure); config options that are not linux_perf.

**Actual code path**: Config created, linux_perf flag set on it, engine successfully initialized via `wasm_runtime_full_init` path (SUCCESS path), then engine properly deleted.

**Path type**: SUCCESS

### Alignment: YES

The test name states linux_perf config creates engine successfully, and coverage confirms the full SUCCESS path through engine creation with that config option is exercised.

### Quality Screening

None.

---

## Test Case [2/9]: EngineStoreTest.EngineConfig_WithSegueFlags_CreatesSuccessfully

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 47
**Parameterized**: No

### Coverage
- Lines: 2.1% (338/15824)
- Functions: 4.1% (53/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_config_set_segue_flags` / `wasm_engine_new_with_config`
**Intended scenario**: Create a config with segue flags set (0x1F1F), then create an engine; verifies engine creation succeeds with segue configuration.
**Intended outcome**: Both `wasm_config_set_segue_flags` and engine creation return non-null.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_config_set_segue_flags` in `wasm_c_api.c` (called once)
- `wasm_engine_new_with_config` in `wasm_c_api.c` (called once)
- `wasm_engine_new_internal` in `wasm_c_api.c` (called once)
- `wasm_engine_delete` in `wasm_c_api.c` (called once)

**Uncovered paths**: Error paths in `wasm_engine_new_internal`; other config option setters.

**Actual code path**: Segue flags written to config struct, engine fully initialized on SUCCESS path, then deleted.

**Path type**: SUCCESS

### Alignment: YES

Test name correctly reflects that a segue-flags config is exercised through the engine creation success path.

### Quality Screening

None.

---

## Test Case [3/9]: EngineStoreTest.Store_WithNullEngine_FailsGracefully

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 61
**Parameterized**: No

### Coverage
- Lines: 0.1% (7/15824)
- Functions: 0.2% (2/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_store_new`
**Intended scenario**: Pass `nullptr` as the engine argument to `wasm_store_new` and expect null to be returned.
**Intended outcome**: `ASSERT_EQ(nullptr, store)` — store creation fails gracefully.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_store_new` in `wasm_c_api.c` (called once)

**Uncovered paths**: All code after the early null-check return (`if (!engine || singleton_engine != engine) return NULL`); the success path allocation branches are never reached.

**Actual code path**: `wasm_store_new` enters the null guard (`!engine`) at line 666 and returns NULL immediately — a pure FAILURE/guard path.

**Path type**: FAILURE

### Alignment: YES

Test name says "FailsGracefully" for a null-engine argument, and coverage confirms only the early null-return guard inside `wasm_store_new` is covered.

### Quality Screening

None.

---

## Test Case [4/9]: EngineStoreTest.Store_DeletedEngine_HandlesGracefully

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 67
**Parameterized**: No

### Coverage
- Lines: 2.6% (413/15824)
- Functions: 5.5% (71/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_engine_delete` / `wasm_store_delete`
**Intended scenario**: Create an engine and store normally, then delete the engine first, followed by deleting the store. Tests whether the teardown order does not crash.
**Intended outcome**: No crash — both deletes complete cleanly; test passes without assertion failure.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_engine_new` in `wasm_c_api.c` (called once)
- `wasm_store_new` in `wasm_c_api.c` (called once)
- `wasm_engine_delete` in `wasm_c_api.c` (called once)
- `wasm_store_delete` in `wasm_c_api.c` (called once)
- `wasm_engine_delete_internal` in `wasm_c_api.c` (called once — engine fully destroyed)
- `decrease_thread_local_store_num` in `wasm_c_api.c` (called once)

**Uncovered paths**: The `decrease_thread_local_store_num` path inside `wasm_store_delete` that calls `wasm_runtime_destroy_thread_env` after the engine has been freed (potential use-after-free of `singleton_engine` in `wasm_store_delete` line 737).

**Actual code path**: Engine and store both created successfully; engine deleted (singleton freed, `singleton_engine = NULL`); store deleted — this hits the `wasm_store_delete` path which references `singleton_engine` after it was nulled. The test passes but covers a potentially dangerous ordering.

**Path type**: EDGE

### Alignment: YES

Test name says "DeletedEngine_HandlesGracefully" and coverage shows the edge-case teardown ordering path is exercised.

### Quality Screening

None.

---

## Test Case [5/9]: EngineStoreTest.Store_ConcurrentCreation_ThreadSafe

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 80
**Parameterized**: No

### Coverage
- Lines: 2.4% (375/15824)
- Functions: 5.1% (66/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_store_new` (concurrent/thread-safety)
**Intended scenario**: Spawn 4 threads each calling `wasm_store_new` on the same engine concurrently; verify all 4 stores are created and are distinct pointers.
**Intended outcome**: All `stores[i]` non-null, all pointers distinct from each other.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_store_new` in `wasm_c_api.c` (called 4 times — one per thread)
- `wasm_runtime_init_thread_env` (called 4 times)
- `increase_thread_local_store_num` (called 4 times)
- `retrieve_thread_local_store_num` (called 4 times)
- `wasm_store_delete` (called 4 times)
- `os_thread_signal_init` (called 5 times — includes main thread)

**Uncovered paths**: Paths inside `wasm_store_new` where `retrieve_thread_local_store_num` returns true (existing thread already has a store — not triggered since each thread is new).

**Actual code path**: Engine created once; 4 new threads each get their own thread-local store allocation, successfully executing the "new thread" branch of `wasm_store_new`; stores are confirmed distinct. This is the SUCCESS multi-thread path.

**Path type**: SUCCESS

### Alignment: YES

Test name says "ConcurrentCreation_ThreadSafe" and coverage confirms 4 concurrent `wasm_store_new` calls on separate threads all succeed.

### Quality Screening

None.

---

## Test Case [6/9]: EngineStoreTest.Engine_DoubleDelete_HandlesGracefully

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 114
**Parameterized**: No

### Coverage
- Lines: 2.1% (340/15824)
- Functions: 4.1% (53/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_engine_delete`
**Intended scenario**: Delete an engine twice in succession; the second delete should be a no-op that does not crash.
**Intended outcome**: No crash; test completes normally.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_engine_delete` in `wasm_c_api.c` (called 2 times)
- `wasm_engine_delete_internal` in `wasm_c_api.c` (called 1 time — only on first delete)

**Uncovered paths**: The `!engine` early-return guard at the top of `wasm_engine_delete` (test passes a valid non-null pointer both times).

**Actual code path**: First `wasm_engine_delete` decrements refcount from 1 to 0 and calls `wasm_engine_delete_internal`, setting `singleton_engine = NULL`. Second `wasm_engine_delete` hits the `!singleton_engine` guard and returns immediately — the EDGE guard path.

**Path type**: EDGE

### Alignment: YES

Test name says "DoubleDelete_HandlesGracefully" and coverage confirms both the full-deletion path and the subsequent no-op guard path inside `wasm_engine_delete` are covered.

### Quality Screening

None.

---

## Test Case [7/9]: EngineStoreTest.Engine_DeleteNull_HandlesGracefully

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 124
**Parameterized**: No

### Coverage
- Lines: 0.1% (6/15824)
- Functions: 0.2% (2/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_engine_delete`
**Intended scenario**: Call `wasm_engine_delete(nullptr)` — the null-pointer guard.
**Intended outcome**: No crash.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_engine_delete` in `wasm_c_api.c` (called once)

**Uncovered paths**: All paths after the `if (!engine) return` guard.

**Actual code path**: `wasm_engine_delete` immediately returns on the `!engine` null check — covers only the null-guard EDGE path with minimal lines (6 lines total).

**Path type**: EDGE

### Alignment: YES

Test name says "DeleteNull_HandlesGracefully" and coverage confirms only the null-argument guard branch is exercised.

### Quality Screening

None.

---

## Test Case [8/9]: EngineStoreTest.Store_DeleteNull_HandlesGracefully

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 130
**Parameterized**: No

### Coverage
- Lines: 0.1% (6/15824)
- Functions: 0.2% (2/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_store_delete`
**Intended scenario**: Call `wasm_store_delete(nullptr)` — the null-pointer guard.
**Intended outcome**: No crash.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_store_delete` in `wasm_c_api.c` (called once)

**Uncovered paths**: All paths after the `if (!store) return` guard — the store contents teardown, `decrease_thread_local_store_num`, etc.

**Actual code path**: `wasm_store_delete` immediately returns on the `if (!store)` null check at line 724, covering only the null-guard EDGE path.

**Path type**: EDGE

### Alignment: YES

Test name says "DeleteNull_HandlesGracefully" and coverage confirms only the null-argument early-return guard in `wasm_store_delete` is hit.

### Quality Screening

None.

---

## Test Case [9/9]: EngineStoreTest.Config_MultipleConfigurations_WorkCorrectly

**File**: `llm-enhanced-testwasm-c-api/test_engine_store.cc`
**Start line**: 136
**Parameterized**: No

### Coverage
- Lines: 1.6% (261/15824)
- Functions: 3.0% (39/1290)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasm_engine_new_with_config` (singleton behavior) / `wasm_config_set_mem_alloc_opt` / `wasm_config_set_linux_perf_opt`
**Intended scenario**: Create three different configs (default, pool allocator, linux_perf) and create an engine from each. Since the engine is a singleton, all three `wasm_engine_new_with_config` calls should return the same pointer (ref-count increments). Verify all engines point to the same object.
**Intended outcome**: All engines equal (`ASSERT_EQ(engines[0], engines[i])`); only one `wasm_engine_delete` needed.

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_config_new` in `wasm_c_api.c` (called 3 times)
- `wasm_engine_new_with_config` in `wasm_c_api.c` (called 3 times)
- `wasm_engine_new_internal` in `wasm_c_api.c` (called 1 time — only first call actually initializes)
- `wasm_config_set_mem_alloc_opt` in `wasm_c_api.c` (called 1 time)
- `wasm_config_set_linux_perf_opt` in `wasm_c_api.c` (called 1 time)
- `wasm_engine_delete` in `wasm_c_api.c` (called 1 time)

**Uncovered paths**: The `wasm_engine_new_internal` config options for pool/segue/perf (second and third calls skip internal init since `singleton_engine` is already set, only incrementing refcount).

**Actual code path**: First `wasm_engine_new_with_config` call creates the singleton (calls `wasm_engine_new_internal`); subsequent two calls find `singleton_engine` already set and only increment the refcount, returning the same pointer. All three `engines[i]` equal `engines[0]`. One delete decrements refcount to 0 and frees.

**Path type**: SUCCESS

### Alignment: YES

Test name says "Config_MultipleConfigurations_WorkCorrectly" and coverage confirms the singleton pattern with multiple `wasm_engine_new_with_config` calls is exercised correctly.

### Quality Screening

None.

---

# Path Coverage Summary: test_engine_store.cc

## Function Coverage Analysis

| Target Function | SUCCESS | FAILURE | EDGE | Total | Status |
|-----------------|---------|---------|------|-------|--------|
| `wasm_config_new` | 1 | 0 | 0 | 1 | Missing FAILURE, EDGE |
| `wasm_config_set_linux_perf_opt` | 2 | 0 | 0 | 2 | Missing FAILURE, EDGE |
| `wasm_config_set_segue_flags` | 1 | 0 | 0 | 1 | Missing FAILURE, EDGE |
| `wasm_config_set_mem_alloc_opt` | 1 | 0 | 0 | 1 | Missing FAILURE, EDGE |
| `wasm_engine_new_with_config` | 2 | 0 | 1 | 3 | Missing FAILURE |
| `wasm_engine_new_internal` | 1 | 0 | 0 | 1 | Missing FAILURE, EDGE |
| `wasm_engine_delete` | 1 | 0 | 2 | 3 | Missing FAILURE |
| `wasm_store_new` | 2 | 1 | 0 | 3 | Missing EDGE |
| `wasm_store_delete` | 1 | 0 | 1 | 2 | Missing FAILURE |

**Status Criteria (STRICT):**
- Complete: Function has at least one test for EACH of SUCCESS, FAILURE, and EDGE paths
- Missing X: Function is missing one or more path types - MUST recommend new tests
- Poor: Function has only 1 path type covered - high priority for enhancement

## Enhancement Recommendations

> **Note**: These recommendations are preserved for future reference. The fix agent will **NOT** implement them at this time - there may already be other test files covering the same paths. They serve as a backlog for future coverage improvements.

### `wasm_config_new` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `wasm_config_new_AllocationFails_ReturnsNull`
   - Scenario: Inject OOM / mock allocator to force `malloc_internal` to fail inside `wasm_config_new`
   - Expected: Returns `nullptr`
2. `wasm_config_new_DefaultValues_AreZeroed`
   - Scenario: Call `wasm_config_new`, inspect the returned struct fields for zero/default initialization
   - Expected: All config fields default to zero/false

### `wasm_config_set_linux_perf_opt` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `wasm_config_set_linux_perf_opt_NullConfig_ReturnsNull`
   - Scenario: Pass `nullptr` as the config argument
   - Expected: Returns `nullptr` without crash
2. `wasm_config_set_linux_perf_opt_False_CorrectlyDisables`
   - Scenario: Create config, set linux_perf to `false`, check the flag is cleared
   - Expected: Returns non-null with perf disabled

### `wasm_config_set_segue_flags` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `wasm_config_set_segue_flags_NullConfig_ReturnsNull`
   - Scenario: Pass `nullptr` as the config argument
   - Expected: Returns `nullptr`
2. `wasm_config_set_segue_flags_ZeroFlags_WorksCorrectly`
   - Scenario: Set segue flags to 0 (clear all flags)
   - Expected: Returns non-null, engine creation still succeeds

### `wasm_engine_new_with_config` - Missing FAILURE path

**Suggested test cases**:
1. `wasm_engine_new_with_config_RuntimeInitFails_ReturnsNull`
   - Scenario: Force `wasm_runtime_full_init` to fail (e.g., invalid memory allocator config)
   - Expected: Returns `nullptr`

### `wasm_engine_new_internal` - Missing FAILURE and EDGE paths

**Suggested test cases**:
1. `wasm_engine_new_internal_MallocFails_ReturnsNull`
   - Scenario: Exhaust memory so `malloc_internal` for the engine struct fails
   - Expected: Returns `nullptr`, runtime cleaned up
2. `wasm_engine_new_internal_VectorInitFails_CleansUp`
   - Scenario: Trigger `bh_vector_init` failure for the modules vector
   - Expected: Returns `nullptr` without leak

### `wasm_engine_delete` - Missing FAILURE path

**Suggested test cases**:
1. `wasm_engine_delete_MismatchedEngine_AssertsOrIgnores`
   - Scenario: Create two engine pointers (the real singleton plus a fake stack object), delete the non-singleton
   - Expected: Assertion triggered or graceful no-op (documents behavior)

### `wasm_store_new` - Missing EDGE path

**Suggested test cases**:
1. `wasm_store_new_SecondStoreOnSameThread_ReturnsNew`
   - Scenario: Call `wasm_store_new` twice on the same thread; the second call should take the `retrieve_thread_local_store_num` == true branch
   - Expected: Both stores non-null; second store is distinct from first

### `wasm_store_delete` - Missing FAILURE path

**Suggested test cases**:
1. `wasm_store_delete_AfterEngineDestroyed_DoesNotCrash`
   - Scenario: Delete engine then delete store (already covered by test 4 but without assertions on internal state); verify no use-after-free on `singleton_engine` reference inside `wasm_store_delete`
   - Expected: No crash; documents the dangerous ordering behavior

---

# Quality Issues Summary: test_engine_store.cc

No quality issues found.

---

# Static Analysis: test_engine_store.cc

## clang-tidy Results

| Line | Category | Message |
|------|----------|---------|
| 90 | performance-inefficient-vector-operation | `emplace_back` is called inside a loop; consider pre-allocating the container capacity before the loop |
| 148 | modernize-use-nullptr | use `nullptr` instead of `{0}` for zero-initialization of pointer-containing struct |

**Summary**: 2 warnings treated as errors, 0 additional warnings
