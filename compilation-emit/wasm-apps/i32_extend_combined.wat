(module
  (func $test_i32_extend8s (param $x i32) (result i32)
    local.get $x
    i32.extend8_s
  )
  (func $test_i32_extend16s (param $x i32) (result i32)
    local.get $x
    i32.extend16_s
  )
  (func $test_combined (param $x i32) (result i32)
    local.get $x
    i32.extend8_s
    i32.extend16_s
  )
  (export "test_i32_extend8s" (func $test_i32_extend8s))
  (export "test_i32_extend16s" (func $test_i32_extend16s))
  (export "test_combined" (func $test_combined))
)