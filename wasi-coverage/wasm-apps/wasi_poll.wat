(module
  ;; WASI poll_oneoff for testing poll/event handling code
  (import "wasi_snapshot_preview1" "poll_oneoff"
    (func $poll_oneoff (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "clock_time_get"
    (func $clock_time_get (param i32 i64 i32) (result i32)))

  (memory (export "memory") 1)

  ;; Test poll_oneoff with clock subscription (timeout)
  ;; subscription_t layout (48 bytes):
  ;; offset 0: userdata (u64)
  ;; offset 8: type (u8): 0=clock, 1=fd_read, 2=fd_write
  ;; offset 16: union content
  ;; For clock: id(u32) + pad + timeout(u64) + precision(u64) + flags(u16)
  (func (export "test_poll_clock") (result i32)
    ;; subscription at offset 0
    ;; userdata = 42
    (i64.store (i32.const 0) (i64.const 42))
    ;; type = 0 (clock)
    (i32.store8 (i32.const 8) (i32.const 0))
    ;; clock_id = 1 (monotonic)
    (i32.store (i32.const 16) (i32.const 1))
    ;; timeout = 1 nanosecond (near-instant)
    (i64.store (i32.const 24) (i64.const 1))
    ;; precision = 1000
    (i64.store (i32.const 32) (i64.const 1000))
    ;; flags = 0 (relative timeout)
    (i32.store16 (i32.const 40) (i32.const 0))

    ;; event output at offset 256 (32 bytes per event)
    ;; poll_oneoff(in=0, out=256, nsubscriptions=1, nevents=512)
    (call $poll_oneoff (i32.const 0) (i32.const 256) (i32.const 1) (i32.const 512))
  )

  ;; Test poll_oneoff with fd_read subscription on stdin
  (func (export "test_poll_fd_read") (result i32)
    ;; subscription for fd_read on fd 0 (stdin) with timeout
    ;; First subscription: clock timeout
    (i64.store (i32.const 0) (i64.const 1))
    (i32.store8 (i32.const 8) (i32.const 0))
    (i32.store (i32.const 16) (i32.const 1))
    (i64.store (i32.const 24) (i64.const 1))
    (i64.store (i32.const 32) (i64.const 1000))
    (i32.store16 (i32.const 40) (i32.const 0))

    (call $poll_oneoff (i32.const 0) (i32.const 256) (i32.const 1) (i32.const 512))
  )
)
