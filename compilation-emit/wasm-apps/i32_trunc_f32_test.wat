(module
  ;; Function that uses i32.trunc_f32_s operation (signed truncation)
  (func $test_i32_trunc_f32_s (param $p0 f32) (result i32)
    local.get $p0
    i32.trunc_f32_s
  )

  ;; Function that uses i32.trunc_f32_u operation (unsigned truncation)
  (func $test_i32_trunc_f32_u (param $p0 f32) (result i32)
    local.get $p0
    i32.trunc_f32_u
  )

  ;; Function that uses saturating i32.trunc_sat_f32_s operation
  (func $test_i32_trunc_sat_f32_s (param $p0 f32) (result i32)
    local.get $p0
    i32.trunc_sat_f32_s
  )

  ;; Function that uses saturating i32.trunc_sat_f32_u operation
  (func $test_i32_trunc_sat_f32_u (param $p0 f32) (result i32)
    local.get $p0
    i32.trunc_sat_f32_u
  )

  ;; Function with multiple truncation operations
  (func $test_multiple_truncs (param $p0 f32) (param $p1 f32) (result i32)
    local.get $p0
    i32.trunc_f32_s
    local.get $p1
    i32.trunc_f32_u
    i32.add
  )

  ;; Export functions for testing
  (export "test_i32_trunc_f32_s" (func $test_i32_trunc_f32_s))
  (export "test_i32_trunc_f32_u" (func $test_i32_trunc_f32_u))
  (export "test_i32_trunc_sat_f32_s" (func $test_i32_trunc_sat_f32_s))
  (export "test_i32_trunc_sat_f32_u" (func $test_i32_trunc_sat_f32_u))
  (export "test_multiple_truncs" (func $test_multiple_truncs))
)