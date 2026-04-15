(module
  ;; Function that uses i32.wrap_i64 operation
  (func $test_i32_wrap_i64 (param $p0 i64) (result i32)
    local.get $p0
    i32.wrap_i64
  )

  ;; Function with multiple i32.wrap_i64 operations
  (func $test_multiple_wraps (param $p0 i64) (param $p1 i64) (result i32)
    local.get $p0
    i32.wrap_i64
    local.get $p1
    i32.wrap_i64
    i32.add
  )

  ;; Export functions for testing
  (export "test_i32_wrap_i64" (func $test_i32_wrap_i64))
  (export "test_multiple_wraps" (func $test_multiple_wraps))
)