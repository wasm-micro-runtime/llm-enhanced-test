(module
  (func $test_i64_extend32_s (param $0 i64) (result i64)
    local.get $0
    i64.extend32_s
  )
  (export "test_i64_extend32_s" (func $test_i64_extend32_s))
)