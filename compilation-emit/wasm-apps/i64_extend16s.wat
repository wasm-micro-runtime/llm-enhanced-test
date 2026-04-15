(module
  (func $test_i64_extend16_s (param $0 i64) (result i64)
    local.get $0
    i64.extend16_s
  )
  (export "test_i64_extend16_s" (func $test_i64_extend16_s))
)