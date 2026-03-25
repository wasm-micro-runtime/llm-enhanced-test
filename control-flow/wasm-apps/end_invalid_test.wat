(module
  ;; Malformed module with unmatched 'end' instruction
  (func $unmatched_end (result i32)
    (block (result i32)
      i32.const 42
    )
    end  ;; Extra 'end' instruction without matching block/if/loop
    i32.const 0
  )
)