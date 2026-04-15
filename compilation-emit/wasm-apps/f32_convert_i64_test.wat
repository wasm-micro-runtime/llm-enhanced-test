(module
  (func $f32_convert_i64_s_test (param $value i64) (result f32)
    local.get $value
    f32.convert_i64_s
  )

  (func $f32_convert_i64_u_test (param $value i64) (result f32)
    local.get $value
    f32.convert_i64_u
  )

  (func $combined_convert_test (param $signed_val i64) (param $unsigned_val i64) (result f32)
    ;; Test signed conversion
    local.get $signed_val
    f32.convert_i64_s

    ;; Test unsigned conversion
    local.get $unsigned_val
    f32.convert_i64_u

    ;; Add results (simple combination to use both)
    f32.add
  )

  (export "f32_convert_i64_s_test" (func $f32_convert_i64_s_test))
  (export "f32_convert_i64_u_test" (func $f32_convert_i64_u_test))
  (export "combined_convert_test" (func $combined_convert_test))
)