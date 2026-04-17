(module
  (type (;0;) (func (result f32)))
  (type (;1;) (func (result f64)))
  (type (;2;) (func (result i32)))

  ;; f32 constant function - used for thumb architecture intrinsic test
  (func (;0;) (type 0) (result f32)
    f32.const 1.0)

  ;; f32 zero constant
  (func (;1;) (type 0) (result f32)
    f32.const 0.0)

  ;; f64 constant function
  (func (;2;) (type 1) (result f64)
    f64.const 1.0)

  ;; i32 constant function
  (func (;3;) (type 2) (result i32)
    i32.const 42)

  (export "get_f32" (func 0))
  (export "get_f32_zero" (func 1))
  (export "get_f64" (func 2))
  (export "get_i32" (func 3))
)
