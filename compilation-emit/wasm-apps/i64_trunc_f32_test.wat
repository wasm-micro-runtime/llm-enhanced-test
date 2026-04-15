(module
  (func (export "i64_trunc_f32_s") (param f32) (result i64)
    local.get 0
    i64.trunc_f32_s
  )
  (func (export "i64_trunc_f32_u") (param f32) (result i64)
    local.get 0
    i64.trunc_f32_u
  )
  (func (export "i64_trunc_sat_f32_s") (param f32) (result i64)
    local.get 0
    i64.trunc_sat_f32_s
  )
  (func (export "i64_trunc_sat_f32_u") (param f32) (result i64)
    local.get 0
    i64.trunc_sat_f32_u
  )
  (func (export "combined_operations") (param f32 f32 f32 f32) (result i64 i64 i64 i64)
    local.get 0
    i64.trunc_f32_s
    local.get 1
    i64.trunc_f32_u
    local.get 2
    i64.trunc_sat_f32_s
    local.get 3
    i64.trunc_sat_f32_u
  )
)