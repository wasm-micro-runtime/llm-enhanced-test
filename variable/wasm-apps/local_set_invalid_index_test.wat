;; Invalid local.set index test - crafted to fail module validation
;; This module uses local.set with index 99 but only 1 param (index 0) exists.
;; WAT tools reject this at compile time, so the .wasm is crafted as a raw binary.
;; The binary encodes: func (param i32) { i32.const 42; local.set 99 }
;; WAMR's wasm_loader_prepare_bytecode rejects it with "unknown local".
(module
  ;; Intentionally invalid: local.set 99 with only param 0 available
  ;; Cannot be compiled by wat2wasm (invalid local index caught at WAT level)
  ;; The .wasm binary is hand-crafted to embed this invalid bytecode
)
