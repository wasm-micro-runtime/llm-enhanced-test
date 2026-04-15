(module
  ;; WASI imports for environment and arguments
  (import "wasi_snapshot_preview1" "args_get"
    (func $args_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "args_sizes_get"
    (func $args_sizes_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "environ_get"
    (func $environ_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "environ_sizes_get"
    (func $environ_sizes_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "clock_res_get"
    (func $clock_res_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "clock_time_get"
    (func $clock_time_get (param i32 i64 i32) (result i32)))
  (import "wasi_snapshot_preview1" "random_get"
    (func $random_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "sched_yield"
    (func $sched_yield (result i32)))
  (import "wasi_snapshot_preview1" "proc_raise"
    (func $proc_raise (param i32) (result i32)))

  (memory (export "memory") 2)

  ;; Test args_sizes_get
  (func (export "test_args_sizes_get") (result i32)
    (call $args_sizes_get (i32.const 0) (i32.const 4))
  )

  ;; Test args_get
  (func (export "test_args_get") (result i32)
    ;; First get sizes
    (drop (call $args_sizes_get (i32.const 0) (i32.const 4)))
    ;; args_get(argv_ptrs=1024, argv_buf=4096)
    (call $args_get (i32.const 1024) (i32.const 4096))
  )

  ;; Test environ_sizes_get
  (func (export "test_environ_sizes_get") (result i32)
    (call $environ_sizes_get (i32.const 0) (i32.const 4))
  )

  ;; Test environ_get
  (func (export "test_environ_get") (result i32)
    (drop (call $environ_sizes_get (i32.const 0) (i32.const 4)))
    (call $environ_get (i32.const 1024) (i32.const 4096))
  )

  ;; Test clock_res_get (realtime clock)
  (func (export "test_clock_res_get_realtime") (result i32)
    (call $clock_res_get (i32.const 0) (i32.const 512))
  )

  ;; Test clock_res_get (monotonic clock)
  (func (export "test_clock_res_get_monotonic") (result i32)
    (call $clock_res_get (i32.const 1) (i32.const 512))
  )

  ;; Test clock_time_get (realtime)
  (func (export "test_clock_time_get_realtime") (result i32)
    (call $clock_time_get (i32.const 0) (i64.const 1000) (i32.const 512))
  )

  ;; Test clock_time_get (monotonic)
  (func (export "test_clock_time_get_monotonic") (result i32)
    (call $clock_time_get (i32.const 1) (i64.const 1000) (i32.const 512))
  )

  ;; Test random_get
  (func (export "test_random_get") (result i32)
    (call $random_get (i32.const 512) (i32.const 32))
  )

  ;; Test sched_yield
  (func (export "test_sched_yield") (result i32)
    (call $sched_yield)
  )
)
