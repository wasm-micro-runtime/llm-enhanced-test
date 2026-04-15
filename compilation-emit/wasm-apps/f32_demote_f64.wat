(module
  (func $f32_demote_f64_test (export "f32_demote_f64_test") (param f64) (result f32)
    local.get 0
    f32.demote_f64
  )

  (func $test_multiple_demote (export "test_multiple_demote") (result f32)
    f64.const 3.14159265359
    f32.demote_f64
    f64.const 2.71828182846
    f32.demote_f64
    f32.add
  )
)