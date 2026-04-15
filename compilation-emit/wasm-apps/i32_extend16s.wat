(module
  (func $test_i32_extend16s (param $x i32) (result i32)
    local.get $x
    i32.extend16_s
  )
  (export "test_i32_extend16s" (func $test_i32_extend16s))
)