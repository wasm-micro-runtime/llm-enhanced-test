(module
  (type (;0;) (func (result f32)))
  (type (;1;) (func (result f32)))
  (type (;2;) (func (result f32)))

  ;; Function returning normal f32 constant
  (func (;0;) (type 0) (result f32)
    f32.const 3.14159)

  ;; Function returning f32 zero
  (func (;1;) (type 1) (result f32)
    f32.const 0.0)

  ;; Function returning f32 NaN (this should trigger the NaN handling path)
  (func (;2;) (type 2) (result f32)
    f32.const nan)

  ;; Function returning f32 infinity
  (func (;3;) (type 0) (result f32)
    f32.const inf)

  ;; Function returning negative f32 infinity
  (func (;4;) (type 0) (result f32)
    f32.const -inf)

  (export "normal_f32" (func 0))
  (export "zero_f32" (func 1))
  (export "nan_f32" (func 2))
  (export "inf_f32" (func 3))
  (export "neg_inf_f32" (func 4))
)