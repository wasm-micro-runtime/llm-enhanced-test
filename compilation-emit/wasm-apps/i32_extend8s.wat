(module
  (func $test_i32_extend8s (param $x i32) (result i32)
    local.get $x
    i32.extend8_s
  )
  (export "test_i32_extend8s" (func $test_i32_extend8s))
)