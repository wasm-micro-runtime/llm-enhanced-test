(module
  (func $i64_trunc_f64_s_test (export "i64_trunc_f64_s_test") (param f64) (result i64)
    local.get 0
    i64.trunc_f64_s
  )

  (func $i64_trunc_f64_u_test (export "i64_trunc_f64_u_test") (param f64) (result i64)
    local.get 0
    i64.trunc_f64_u
  )

  (func $i64_trunc_sat_f64_s_test (export "i64_trunc_sat_f64_s_test") (param f64) (result i64)
    local.get 0
    i64.trunc_sat_f64_s
  )

  (func $i64_trunc_sat_f64_u_test (export "i64_trunc_sat_f64_u_test") (param f64) (result i64)
    local.get 0
    i64.trunc_sat_f64_u
  )

  (func $combined_i64_trunc_f64_operations (export "combined_i64_trunc_f64_operations") (param f64 f64 f64 f64) (result i64 i64 i64 i64)
    local.get 0
    i64.trunc_f64_s
    local.get 1
    i64.trunc_f64_u
    local.get 2
    i64.trunc_sat_f64_s
    local.get 3
    i64.trunc_sat_f64_u
  )
)