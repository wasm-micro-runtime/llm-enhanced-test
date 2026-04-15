(module
  ;; Function that uses i32.trunc_f64_s operation (signed non-saturating)
  (func $test_i32_trunc_f64_s (param $p0 f64) (result i32)
    local.get $p0
    i32.trunc_f64_s
  )

  ;; Function that uses i32.trunc_f64_u operation (unsigned non-saturating)
  (func $test_i32_trunc_f64_u (param $p0 f64) (result i32)
    local.get $p0
    i32.trunc_f64_u
  )

  ;; Function that uses i32.trunc_sat_f64_s operation (signed saturating)
  (func $test_i32_trunc_sat_f64_s (param $p0 f64) (result i32)
    local.get $p0
    i32.trunc_sat_f64_s
  )

  ;; Function that uses i32.trunc_sat_f64_u operation (unsigned saturating)
  (func $test_i32_trunc_sat_f64_u (param $p0 f64) (result i32)
    local.get $p0
    i32.trunc_sat_f64_u
  )

  ;; Function with mixed f64 truncation operations
  (func $test_mixed_f64_trunc (param $p0 f64) (param $p1 f64) (result i32)
    local.get $p0
    i32.trunc_f64_s
    local.get $p1
    i32.trunc_sat_f64_u
    i32.add
  )

  ;; Export functions for testing
  (export "test_i32_trunc_f64_s" (func $test_i32_trunc_f64_s))
  (export "test_i32_trunc_f64_u" (func $test_i32_trunc_f64_u))
  (export "test_i32_trunc_sat_f64_s" (func $test_i32_trunc_sat_f64_s))
  (export "test_i32_trunc_sat_f64_u" (func $test_i32_trunc_sat_f64_u))
  (export "test_mixed_f64_trunc" (func $test_mixed_f64_trunc))
)