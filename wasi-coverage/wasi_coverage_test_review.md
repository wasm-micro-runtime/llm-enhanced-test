# Test Review Summary: wasi_coverage_test.cc

## Redundancy Cleanup (from check_redundant_tests.js)

- **Original tests:** 34
- **Identified (redundant):** 0
- **Remaining tests (useful):** 34

### Redundant Test Cases (deleted in PHASE 1.5)
| Test Case | Reason | Status |
|-----------|--------|--------|
| (none) | N/A | N/A |

---

## Test Case [1/34]: WasiCoverageTest.FdWriteStdout

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 162
**Parameterized**: No

### Coverage
- Lines: 13.2% (2799/21168)
- Functions: 17.6% (309/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_write` (WASI fd_write system call on stdout)
**Intended scenario**: Loads wasi_fd_ops.wasm module, looks up and calls `test_fd_write_stdout` exported function which internally invokes the fd_write WASI call on file descriptor 1 (stdout)
**Intended outcome**: The WASI fd_write call completes successfully writing to stdout

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA, guided by Expected Purpose above):
- `wasi_fd_write` in `libc_wasi_wrapper.c` (WASI wrapper layer)
- `wasm_interp_call_wasm` in `wasm_interp_classic.c` (interpreter dispatch)
- `call_wasm_with_hw_bound_check` in `wasm_runtime.c` (HW bound check path)
- `wasm_runtime_call_wasm` in `wasm_runtime_common.c` (common runtime call)

**Uncovered paths** (from coverage-summary.md): AOT branch in `wasm_runtime_call_wasm` (line 2754), error paths for invalid exec env (line 2730), argument conversion failure (line 2738), call failure return (line 2758), result conversion failure (line 2767)

**Actual code path**: Module loads, instantiates with WASI args, looks up `test_fd_write_stdout`, calls it through interpreter which dispatches to the `wasi_fd_write` native function. Exception is cleared afterward regardless of result.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdWriteStdout" matches the actual coverage of `wasi_fd_write` through the interpreter success path.

### Quality Screening

- Missing assertions: No assertion on the return value of `wasm_runtime_call_wasm` - exceptions are silently cleared with `wasm_runtime_clear_exception`
- No verification that fd_write actually wrote data to stdout

---

## Test Case [2/34]: WasiCoverageTest.FdWriteStderr

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 167
**Parameterized**: No

### Coverage
- Lines: 13.2% (2799/21168)
- Functions: 17.6% (309/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_write` (WASI fd_write system call on stderr)
**Intended scenario**: Loads wasi_fd_ops.wasm module, calls `test_fd_write_stderr` which invokes fd_write on file descriptor 2 (stderr)
**Intended outcome**: The WASI fd_write call completes successfully writing to stderr

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_write` in `libc_wasi_wrapper.c`
- Same infrastructure functions as FdWriteStdout

**Uncovered paths**: Same as FdWriteStdout - AOT branch, error paths

**Actual code path**: Identical to FdWriteStdout but targeting fd 2 (stderr). Same `wasi_fd_write` function is exercised.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdWriteStderr" matches the actual coverage of `wasi_fd_write` targeting stderr.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value
- No verification that fd_write actually wrote data to stderr

---

## Test Case [3/34]: WasiCoverageTest.FdFdstatGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 172
**Parameterized**: No

### Coverage
- Lines: 13.0% (2748/21168)
- Functions: 17.2% (302/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_fdstat_get` (WASI fd_fdstat_get system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_fdstat_get` which invokes fd_fdstat_get to get file descriptor status
**Intended outcome**: Successfully retrieves fd stat information

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_fdstat_get` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths as other tests

**Actual code path**: Module loads, instantiates, calls test function through interpreter which dispatches to `wasi_fd_fdstat_get`.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdFdstatGet" matches actual coverage of `wasi_fd_fdstat_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [4/34]: WasiCoverageTest.FdPrestatGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 177
**Parameterized**: No

### Coverage
- Lines: 12.9% (2733/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_prestat_get` (WASI fd_prestat_get system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_prestat_get` which invokes fd_prestat_get to get preopen directory info
**Intended outcome**: Successfully retrieves prestat information for preopened directories

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_prestat_get` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_prestats` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_prestat_get` which accesses the prestats table.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdPrestatGet" matches actual coverage of `wasi_fd_prestat_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [5/34]: WasiCoverageTest.FdPrestatDirName

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 182
**Parameterized**: No

### Coverage
- Lines: 12.8% (2717/21168)
- Functions: 17.1% (300/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_prestat_dir_name` (WASI fd_prestat_dir_name system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_prestat_dir_name` which retrieves the preopened directory name
**Intended outcome**: Successfully retrieves prestat directory name

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_prestat_dir_name` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_prestats` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_prestat_dir_name` to get the directory path.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdPrestatDirName" matches actual coverage of `wasi_fd_prestat_dir_name`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [6/34]: WasiCoverageTest.FdFilestatGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 187
**Parameterized**: No

### Coverage
- Lines: 13.0% (2746/21168)
- Functions: 17.2% (302/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_filestat_get` (WASI fd_filestat_get system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_filestat_get` to get file stat info for an fd
**Intended outcome**: Successfully retrieves file statistics

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_filestat_get` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_filestat_get` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdFilestatGet" matches actual coverage of `wasi_fd_filestat_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [7/34]: WasiCoverageTest.FdSync

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 192
**Parameterized**: No

### Coverage
- Lines: 12.8% (2700/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_sync` (WASI fd_sync system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_sync` to sync file data to disk
**Intended outcome**: Successfully syncs file descriptor

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_sync` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_sync` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdSync" matches actual coverage of `wasi_fd_sync`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [8/34]: WasiCoverageTest.FdDatasync

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 197
**Parameterized**: No

### Coverage
- Lines: 12.8% (2700/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_datasync` (WASI fd_datasync system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_datasync` to sync file data (without metadata)
**Intended outcome**: Successfully performs data sync

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_datasync` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_datasync` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdDatasync" matches actual coverage of `wasi_fd_datasync`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [9/34]: WasiCoverageTest.FdFdstatSetFlags

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 202
**Parameterized**: No

### Coverage
- Lines: 12.7% (2682/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_fdstat_set_flags` (WASI fd_fdstat_set_flags system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_fdstat_set_flags` to set flags on an fd
**Intended outcome**: Successfully sets file descriptor flags

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_fdstat_set_flags` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_fdstat_set_flags` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdFdstatSetFlags" matches actual coverage of `wasi_fd_fdstat_set_flags`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [10/34]: WasiCoverageTest.FdSeek

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 207
**Parameterized**: No

### Coverage
- Lines: 13.0% (2744/21168)
- Functions: 17.2% (302/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_seek` (WASI fd_seek system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_seek` to seek within a file
**Intended outcome**: Successfully performs file seek

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_seek` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_seek` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdSeek" matches actual coverage of `wasi_fd_seek`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [11/34]: WasiCoverageTest.FdTell

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 212
**Parameterized**: No

### Coverage
- Lines: 12.9% (2735/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_tell` (WASI fd_tell system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_tell` to get current file position
**Intended outcome**: Successfully gets current position

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_tell` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_tell` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdTell" matches actual coverage of `wasi_fd_tell`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [12/34]: WasiCoverageTest.FdAdvise

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 217
**Parameterized**: No

### Coverage
- Lines: 12.9% (2725/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_advise` (WASI fd_advise system call)
**Intended scenario**: Loads wasi_fd_ops.wasm, calls `test_fd_advise` to advise the OS about file access patterns
**Intended outcome**: Successfully provides file access advice

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_advise` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_advise` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdAdvise" matches actual coverage of `wasi_fd_advise`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [13/34]: WasiCoverageTest.AllFdOps

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 222
**Parameterized**: No

### Coverage
- Lines: 14.3% (3025/21168)
- Functions: 19.2% (338/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: All WASI fd operations (all exports from wasi_fd_ops.wasm)
**Intended scenario**: Uses `run_all_exports` to iterate over all exported functions in wasi_fd_ops.wasm, calling each one. Also exercises `wasm_runtime_get_export_count` and `wasm_runtime_get_export_type`.
**Intended outcome**: All exported functions run successfully

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_runtime_get_export_count` in `wasm_runtime_common.c` (1 call)
- `wasm_runtime_get_export_type` in `wasm_runtime_common.c` (13 calls)
- `get_export_count` in `wasm_runtime.c` (1 call)
- All `wasi_fd_*` functions in `libc_wasi_wrapper.c` via interpreter
- `wasm_lookup_function` in `wasm_runtime.c` (14 calls)

**Uncovered paths** (from coverage-summary.md): AOT branches in `wasm_runtime_get_export_count` (lines 4463-4464), null checks in `wasm_runtime_get_export_type` (lines 4482-4490), AOT type handling in export_type (lines 4495-4549+)

**Actual code path**: Iterates all exports, skips non-function and "memory" exports, calls each function through interpreter. Coverage is a superset of individual FD tests.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "AllFdOps" accurately describes running all FD operation exports.

### Quality Screening

- Missing assertions: No assertion on any `wasm_runtime_call_wasm` return values - all exceptions silently cleared

---

## Test Case [14/34]: WasiCoverageTest.ArgsSizesGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 227
**Parameterized**: No

### Coverage
- Lines: 12.7% (2692/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_args_sizes_get` (WASI args_sizes_get system call)
**Intended scenario**: Loads wasi_env_args.wasm, calls `test_args_sizes_get` to get argument count and buffer size
**Intended outcome**: Successfully retrieves argument sizes

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_args_sizes_get` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_args_sizes_get` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "ArgsSizesGet" matches actual coverage of `wasi_args_sizes_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [15/34]: WasiCoverageTest.ArgsGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 232
**Parameterized**: No

### Coverage
- Lines: 13.0% (2750/21168)
- Functions: 17.2% (302/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_args_get` (WASI args_get system call)
**Intended scenario**: Loads wasi_env_args.wasm, calls `test_args_get` to retrieve the command-line arguments
**Intended outcome**: Successfully retrieves arguments

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_args_get` in `libc_wasi_wrapper.c`
- `wasi_args_sizes_get` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_argv_environ` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_args_get` which also invokes `wasi_args_sizes_get` and accesses argv/environ context.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "ArgsGet" matches actual coverage of `wasi_args_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [16/34]: WasiCoverageTest.EnvironSizesGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 237
**Parameterized**: No

### Coverage
- Lines: 12.7% (2695/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_environ_sizes_get` (WASI environ_sizes_get system call)
**Intended scenario**: Loads wasi_env_args.wasm, calls `test_environ_sizes_get` to get environment variable count and buffer size
**Intended outcome**: Successfully retrieves environment sizes

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_environ_sizes_get` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_argv_environ` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_environ_sizes_get` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "EnvironSizesGet" matches actual coverage of `wasi_environ_sizes_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [17/34]: WasiCoverageTest.EnvironGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 242
**Parameterized**: No

### Coverage
- Lines: 13.0% (2750/21168)
- Functions: 17.2% (302/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_environ_get` (WASI environ_get system call)
**Intended scenario**: Loads wasi_env_args.wasm, calls `test_environ_get` to retrieve environment variables
**Intended outcome**: Successfully retrieves environment variables

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_environ_get` in `libc_wasi_wrapper.c`
- `wasi_environ_sizes_get` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_argv_environ` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_environ_get` which also uses environ_sizes_get and argv_environ context.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "EnvironGet" matches actual coverage of `wasi_environ_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [18/34]: WasiCoverageTest.ClockResGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 247
**Parameterized**: No

### Coverage
- Lines: 12.8% (2709/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_clock_res_get` (WASI clock_res_get system call)
**Intended scenario**: Loads wasi_env_args.wasm, calls `test_clock_res_get_realtime` to get clock resolution
**Intended outcome**: Successfully retrieves clock resolution

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_clock_res_get` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_clock_res_get` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "ClockResGet" matches actual coverage of `wasi_clock_res_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [19/34]: WasiCoverageTest.ClockTimeGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 252
**Parameterized**: No

### Coverage
- Lines: 12.8% (2717/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_clock_time_get` (WASI clock_time_get system call)
**Intended scenario**: Loads wasi_env_args.wasm, calls `test_clock_time_get_realtime` to get current time
**Intended outcome**: Successfully retrieves clock time

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_clock_time_get` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_clock_time_get` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "ClockTimeGet" matches actual coverage of `wasi_clock_time_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [20/34]: WasiCoverageTest.RandomGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 257
**Parameterized**: No

### Coverage
- Lines: 12.7% (2687/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_random_get` (WASI random_get system call)
**Intended scenario**: Loads wasi_env_args.wasm, calls `test_random_get` to get random bytes
**Intended outcome**: Successfully retrieves random data

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_random_get` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_random_get` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "RandomGet" matches actual coverage of `wasi_random_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [21/34]: WasiCoverageTest.SchedYield

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 262
**Parameterized**: No

### Coverage
- Lines: 12.4% (2618/21168)
- Functions: 17.1% (301/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_sched_yield` (WASI sched_yield system call)
**Intended scenario**: Loads wasi_env_args.wasm, calls `test_sched_yield` to yield the scheduler
**Intended outcome**: Successfully yields

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_sched_yield` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_sched_yield` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "SchedYield" matches actual coverage of `wasi_sched_yield`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [22/34]: WasiCoverageTest.AllEnvArgs

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 267
**Parameterized**: No

### Coverage
- Lines: 13.6% (2885/21168)
- Functions: 18.0% (317/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: All env/args WASI operations (all exports from wasi_env_args.wasm)
**Intended scenario**: Uses `run_all_exports` to iterate and call all exported functions. Also exercises `wasm_runtime_get_export_count` and `wasm_runtime_get_export_type`.
**Intended outcome**: All exported functions run successfully

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_runtime_get_export_count` in `wasm_runtime_common.c`
- `wasm_runtime_get_export_type` in `wasm_runtime_common.c`
- All `wasi_args_*`, `wasi_environ_*`, `wasi_clock_*`, `wasi_random_get`, `wasi_sched_yield` in `libc_wasi_wrapper.c`

**Uncovered paths**: AOT branches in export count/type, null checks

**Actual code path**: Iterates all exports from wasi_env_args.wasm and calls each. Coverage is a superset of individual env/args tests.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "AllEnvArgs" accurately describes running all environment/arguments exports.

### Quality Screening

- Missing assertions: No assertion on any `wasm_runtime_call_wasm` return values

---

## Test Case [23/34]: WasiCoverageTest.PathCreateDirectory

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 272
**Parameterized**: No

### Coverage
- Lines: 14.4% (3056/21168)
- Functions: 17.6% (310/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_path_create_directory` (WASI path_create_directory system call)
**Intended scenario**: Loads wasi_path_ops.wasm, calls `test_path_create_directory` to create a directory via WASI
**Intended outcome**: Successfully creates a directory

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_path_create_directory` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`
- Various `wasmtime_ssp_*` and `fd_*` functions in `posix.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_path_create_directory` which goes through sandboxed system primitives to create a directory on the filesystem.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "PathCreateDirectory" matches actual coverage of `wasi_path_create_directory`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [24/34]: WasiCoverageTest.PathOpenCreate

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 277
**Parameterized**: No

### Coverage
- Lines: 14.4% (3051/21168)
- Functions: 17.6% (309/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_path_open` (WASI path_open system call with create flag)
**Intended scenario**: Loads wasi_path_ops.wasm, calls `test_path_open_create` to open/create a file via WASI
**Intended outcome**: Successfully opens/creates a file

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_path_open` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_path_open` with create flags through sandboxed system primitives.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "PathOpenCreate" matches actual coverage of `wasi_path_open` with create semantics.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [25/34]: WasiCoverageTest.FileWriteRead

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 282
**Parameterized**: No

### Coverage
- Lines: 14.8% (3124/21168)
- Functions: 17.6% (310/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_path_open`, `wasi_fd_write`, `wasi_fd_read` (combined file I/O)
**Intended scenario**: Loads wasi_path_ops.wasm, calls `test_file_write_read` which opens a file, writes data, reads it back
**Intended outcome**: Successfully performs write then read

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_path_open` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`
- File I/O functions in `posix.c` (fd_table_attach, fd_object_get, etc.)

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Opens a file, writes data, reads it back - exercises the full file I/O path through WASI.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FileWriteRead" matches actual file write/read coverage.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value or data integrity

---

## Test Case [26/34]: WasiCoverageTest.PathFilestatGet

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 287
**Parameterized**: No

### Coverage
- Lines: 14.5% (3074/21168)
- Functions: 17.6% (309/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_path_filestat_get` (WASI path_filestat_get system call)
**Intended scenario**: Loads wasi_path_ops.wasm, calls `test_path_filestat_get` to get file stats by path
**Intended outcome**: Successfully retrieves file statistics for a path

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_path_filestat_get` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_path_filestat_get` through the interpreter and sandboxed primitives.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "PathFilestatGet" matches actual coverage of `wasi_path_filestat_get`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [27/34]: WasiCoverageTest.FdReaddir

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 292
**Parameterized**: No

### Coverage
- Lines: 14.5% (3074/21168)
- Functions: 17.6% (309/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_readdir` (WASI fd_readdir system call)
**Intended scenario**: Loads wasi_path_ops.wasm, calls `test_fd_readdir` to read directory entries
**Intended outcome**: Successfully reads directory entries

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_fd_readdir` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_fd_readdir` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "FdReaddir" matches actual coverage of `wasi_fd_readdir`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [28/34]: WasiCoverageTest.PwritePread

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 297
**Parameterized**: No

### Coverage
- Lines: 14.8% (3124/21168)
- Functions: 17.6% (310/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_fd_pwrite`, `wasi_fd_pread` (WASI positional write/read)
**Intended scenario**: Loads wasi_path_ops.wasm, calls `test_pwrite_pread` to write/read at specific offsets
**Intended outcome**: Successfully performs positional write and read

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_path_open` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Opens a file and performs positional write/read operations through WASI.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "PwritePread" matches actual coverage of positional write/read operations.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value or data integrity

---

## Test Case [29/34]: WasiCoverageTest.PathUnlink

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 302
**Parameterized**: No

### Coverage
- Lines: 14.5% (3061/21168)
- Functions: 17.6% (309/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_path_unlink_file` (WASI path_unlink_file system call)
**Intended scenario**: Loads wasi_path_ops.wasm, calls `test_path_unlink` to delete a file
**Intended outcome**: Successfully unlinks a file

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_path_unlink_file` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_path_unlink_file` through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "PathUnlink" matches actual coverage of `wasi_path_unlink_file`.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [30/34]: WasiCoverageTest.PathRename

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 307
**Parameterized**: No

### Coverage
- Lines: 14.9% (3154/21168)
- Functions: 17.6% (310/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_path_rename` (WASI path_rename system call)
**Intended scenario**: Loads wasi_path_ops.wasm, calls `test_path_rename` to rename a file
**Intended outcome**: Successfully renames a file

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_path_rename` in `libc_wasi_wrapper.c`
- `wasi_fd_close` in `libc_wasi_wrapper.c`
- `wasi_path_open` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_prestats` in `libc_wasi_wrapper.c`
- Various `wasmtime_ssp_*` functions in `posix.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Opens a file, renames it, and cleans up. Exercises multiple WASI functions in sequence.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "PathRename" matches actual coverage of `wasi_path_rename` and related operations.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [31/34]: WasiCoverageTest.AllPathOps

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 312
**Parameterized**: No

### Coverage
- Lines: 16.1% (3401/21168)
- Functions: 20.0% (352/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: All path operations (all exports from wasi_path_ops.wasm)
**Intended scenario**: Uses `run_all_exports` to iterate and call all exported functions in wasi_path_ops.wasm. Also exercises `wasm_runtime_get_export_count` and `wasm_runtime_get_export_type`.
**Intended outcome**: All exported functions run successfully

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_runtime_get_export_count` in `wasm_runtime_common.c`
- `wasm_runtime_get_export_type` in `wasm_runtime_common.c`
- All `wasi_path_*`, `wasi_fd_*` functions via interpreter
- `wasmtime_ssp_*` functions in `posix.c`

**Uncovered paths**: AOT branches in export count/type functions

**Actual code path**: Iterates all exports from wasi_path_ops.wasm, calling each through the interpreter. Highest coverage test in the file (16.1%).

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "AllPathOps" accurately describes running all path operation exports.

### Quality Screening

- Missing assertions: No assertion on any `wasm_runtime_call_wasm` return values

---

## Test Case [32/34]: WasiCoverageTest.PollClock

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 317
**Parameterized**: No

### Coverage
- Lines: 13.1% (2775/21168)
- Functions: 17.2% (302/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_poll_oneoff` (WASI poll_oneoff with clock subscription)
**Intended scenario**: Loads wasi_poll.wasm, calls `test_poll_clock` to poll with a clock timeout
**Intended outcome**: Successfully polls with clock event

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_poll_oneoff` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_poll_oneoff` with a clock subscription through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "PollClock" matches actual coverage of `wasi_poll_oneoff` with clock subscription.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [33/34]: WasiCoverageTest.PollFdRead

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 322
**Parameterized**: No

### Coverage
- Lines: 13.1% (2775/21168)
- Functions: 17.2% (302/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: `wasi_poll_oneoff` (WASI poll_oneoff with fd_read subscription)
**Intended scenario**: Loads wasi_poll.wasm, calls `test_poll_fd_read` to poll for read readiness on an fd
**Intended outcome**: Successfully polls for fd read readiness

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasi_poll_oneoff` in `libc_wasi_wrapper.c`
- `wasi_ctx_get_curfds` in `libc_wasi_wrapper.c`

**Uncovered paths**: Same infrastructure error paths

**Actual code path**: Calls `wasi_poll_oneoff` with an fd_read subscription through the interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "PollFdRead" matches actual coverage of `wasi_poll_oneoff` with fd_read subscription.

### Quality Screening

- Missing assertions: No assertion on `wasm_runtime_call_wasm` return value

---

## Test Case [34/34]: WasiCoverageTest.AllPoll

**File**: `llm-enhanced-test/wasi-coverage/wasi_coverage_test.cc`
**Start line**: 327
**Parameterized**: No

### Coverage
- Lines: 13.3% (2808/21168)
- Functions: 17.3% (304/1759)

### Expected Testing Purpose (from test code - what AI INTENDED to test)

**Intended target**: All poll operations (all exports from wasi_poll.wasm)
**Intended scenario**: Uses `run_all_exports` to iterate and call all exported functions in wasi_poll.wasm. Also exercises `wasm_runtime_get_export_count` and `wasm_runtime_get_export_type`.
**Intended outcome**: All exported functions run successfully

### Real Testing Purpose (from coverage - what IS actually tested)

**Target functions** (from FNDA):
- `wasm_runtime_get_export_count` in `wasm_runtime_common.c`
- `wasm_runtime_get_export_type` in `wasm_runtime_common.c`
- `wasi_poll_oneoff` in `libc_wasi_wrapper.c`

**Uncovered paths**: AOT branches in export count/type functions

**Actual code path**: Iterates all exports from wasi_poll.wasm and calls each through interpreter.

**Path type** (from coverage): SUCCESS

### Alignment: YES

Test name "AllPoll" accurately describes running all poll exports.

### Quality Screening

- Missing assertions: No assertion on any `wasm_runtime_call_wasm` return values

---

# Path Coverage Summary: wasi_coverage_test.cc

## Function Coverage Analysis

| Target Function | SUCCESS | FAILURE | EDGE | Total | Status |
|-----------------|---------|---------|------|-------|--------|
| `wasi_fd_write` | 2 | 0 | 0 | 2 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_fdstat_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_prestat_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_prestat_dir_name` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_filestat_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_sync` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_datasync` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_fdstat_set_flags` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_seek` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_tell` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_advise` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_args_sizes_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_args_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_environ_sizes_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_environ_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_clock_res_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_clock_time_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_random_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_sched_yield` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_path_create_directory` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_path_open` | 3 | 0 | 0 | 3 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_path_filestat_get` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_fd_readdir` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_path_unlink_file` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_path_rename` | 1 | 0 | 0 | 1 | ❌ Poor - Missing FAILURE, EDGE |
| `wasi_poll_oneoff` | 2 | 0 | 0 | 2 | ❌ Poor - Missing FAILURE, EDGE |
| `wasm_runtime_get_export_count` | 3 | 0 | 0 | 3 | ❌ Poor - Missing FAILURE, EDGE |
| `wasm_runtime_get_export_type` | 3 | 0 | 0 | 3 | ❌ Poor - Missing FAILURE, EDGE |
| `wasm_runtime_call_wasm` | 34 | 0 | 0 | 34 | ❌ Poor - Missing FAILURE, EDGE |

**Status Criteria (STRICT):**
- ✅ **Complete**: Function has at least one test for EACH of SUCCESS, FAILURE, and EDGE paths
- ⚠️ **Missing X**: Function is missing one or more path types - MUST recommend new tests
- ❌ **Poor**: Function has only 1 path type covered - high priority for enhancement

## Enhancement Recommendations

> **Note**: These recommendations are preserved for future reference. The fix agent will **NOT** implement them at this time -- there may already be other test files covering the same paths. They serve as a backlog for future coverage improvements.

### All WASI functions - Missing FAILURE and EDGE paths

**Overall pattern**: Every test in this file only exercises the SUCCESS path through pre-built WASM modules. There are no tests for:
- Invalid file descriptors (FAILURE path)
- Invalid buffer sizes / null pointers (EDGE path)
- Permission denied scenarios (FAILURE path)
- Operations on closed fds (FAILURE path)

**Suggested test cases**:

1. `WasiCoverageTest.FdWriteInvalidFd_ReturnsError`
   - Scenario: Call fd_write with an invalid file descriptor (e.g., fd=999)
   - Expected: WASI returns EBADF error code

2. `WasiCoverageTest.FdSeekOnDir_ReturnsError`
   - Scenario: Call fd_seek on a directory fd
   - Expected: WASI returns ESPIPE or equivalent error

3. `WasiCoverageTest.PathOpenNonexistent_ReturnsError`
   - Scenario: Call path_open on a non-existent file without O_CREAT
   - Expected: WASI returns ENOENT error code

4. `WasiCoverageTest.PathCreateDirectoryExists_ReturnsError`
   - Scenario: Call path_create_directory when directory already exists
   - Expected: WASI returns EEXIST error code

5. `WasiCoverageTest.FdReaddir_EmptyDir_ReturnsEmpty`
   - Scenario: Call fd_readdir on an empty directory
   - Expected: Returns 0 entries (EDGE case)

6. `WasiCoverageTest.ClockTimeGet_InvalidClockId_ReturnsError`
   - Scenario: Call clock_time_get with invalid clock ID
   - Expected: WASI returns EINVAL

7. `WasiCoverageTest.RandomGet_ZeroLength_Succeeds`
   - Scenario: Call random_get with zero length buffer
   - Expected: Returns success with no data (EDGE case)

8. `WasiCoverageTest.PollOneoff_EmptySubscriptions_ReturnsError`
   - Scenario: Call poll_oneoff with zero subscriptions
   - Expected: WASI returns EINVAL

9. `WasiCoverageTest.PathRename_NonexistentSource_ReturnsError`
   - Scenario: Call path_rename with non-existent source path
   - Expected: WASI returns ENOENT

10. `WasiCoverageTest.FdPrestatGet_InvalidFd_ReturnsError`
    - Scenario: Call fd_prestat_get with a non-preopened fd
    - Expected: WASI returns EBADF

---

# Quality Issues Summary: wasi_coverage_test.cc

| Test Case | Issue | Recommendation |
|-----------|-------|----------------|
| ALL 34 tests | Missing assertions on `wasm_runtime_call_wasm` return value - exceptions silently cleared | Add assertions checking return value of `wasm_runtime_call_wasm` and/or verify no exception was set before clearing |

**Total**: 34 issues found (same issue across all test cases - no test verifies the WASI call result)

**Detail**: The `run_wasi_module` helper calls `wasm_runtime_call_wasm(env, func, argc, argv)` but does not check the boolean return value. It then immediately calls `wasm_runtime_clear_exception(inst)` which silently swallows any errors. This means tests will pass even if the WASI call fails. The `run_all_exports` helper has the same issue.

**Recommendation**: Modify `run_wasi_module` to assert on the return value:
```cpp
bool ret = wasm_runtime_call_wasm(env, func, argc, argv);
ASSERT_TRUE(ret) << "Call failed: " << wasm_runtime_get_exception(inst);
```

---

# Static Analysis: wasi_coverage_test.cc

## clang-tidy Results

| Line | Category | Message |
|------|----------|---------|
| 49 | cert-err33-c | the value returned by `snprintf` should be used |
| 55 | readability-implicit-bool-conversion | implicit conversion `char` -> bool |
| 57 | cert-err33-c | the value returned by `snprintf` should be used |
| 58 | cert-env33-c | calling `system` uses a command processor |
| 58 | concurrency-mt-unsafe | function is not thread safe |
| 68 | modernize-use-auto | use auto when initializing with a cast to avoid duplicating the type name |
| 80 | modernize-use-nullptr | use nullptr instead of NULL |
| 93 | readability-implicit-bool-conversion | implicit conversion `wasm_function_inst_t` (aka `void *`) -> bool |
| 95 | readability-implicit-bool-conversion | implicit conversion `uint32_t *` (aka `unsigned int *`) -> bool |
| 112 | modernize-use-auto | use auto when initializing with a cast to avoid duplicating the type name |
| 124 | modernize-use-nullptr | use nullptr instead of NULL |
| 147 | readability-implicit-bool-conversion | implicit conversion `wasm_function_inst_t` (aka `void *`) -> bool |

**Summary**: 12 warnings, 0 errors
