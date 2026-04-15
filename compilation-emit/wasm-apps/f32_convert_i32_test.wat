(module
  (func $f32_convert_i32_s_test (export "f32_convert_i32_s_test") (param i32) (result f32)
    local.get 0
    f32.convert_i32_s
  )

  (func $f32_convert_i32_u_test (export "f32_convert_i32_u_test") (param i32) (result f32)
    local.get 0
    f32.convert_i32_u
  )

  (func $combined_f32_convert_i32_operations (export "combined_f32_convert_i32_operations") (param i32 i32) (result f32 f32)
    local.get 0
    f32.convert_i32_s
    local.get 1
    f32.convert_i32_u
  )

  (func $f32_convert_i32_intrinsic_test (export "f32_convert_i32_intrinsic_test") (param i32) (result f32)
    local.get 0
    f32.convert_i32_s
    local.get 0
    f32.convert_i32_u
    f32.add
  )
)