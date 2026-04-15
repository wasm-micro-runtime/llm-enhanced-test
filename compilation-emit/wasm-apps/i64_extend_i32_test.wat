(module
  (func $test_i64_extend_i32_s (param i32) (result i64)
    local.get 0
    i64.extend_i32_s
  )

  (func $test_i64_extend_i32_u (param i32) (result i64)
    local.get 0
    i64.extend_i32_u
  )

  (func $test_combined (param i32) (result i64)
    local.get 0
    i64.extend_i32_s
    local.get 0
    i64.extend_i32_u
    i64.add
  )

  (export "test_i64_extend_i32_s" (func $test_i64_extend_i32_s))
  (export "test_i64_extend_i32_u" (func $test_i64_extend_i32_u))
  (export "test_combined" (func $test_combined))
)