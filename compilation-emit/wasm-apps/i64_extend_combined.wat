(module
  (func $test_combined (param $0 i64) (result i64)
    local.get $0
    i64.extend8_s
    i64.extend16_s
    i64.extend32_s
  )
  (export "test_combined" (func $test_combined))
)