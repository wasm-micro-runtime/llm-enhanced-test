(module
  (func (export "promote_f32_to_f64") (param f32) (result f64)
    local.get 0
    f64.promote_f32
  )

  (func (export "promote_with_ops") (param f32) (result f64)
    local.get 0
    f64.promote_f32
    f64.const 2.0
    f64.mul
  )
)