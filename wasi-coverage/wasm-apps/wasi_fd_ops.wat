(module
  ;; WASI imports for file descriptor operations
  (import "wasi_snapshot_preview1" "fd_write"
    (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_read"
    (func $fd_read (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close"
    (func $fd_close (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_seek"
    (func $fd_seek (param i32 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_tell"
    (func $fd_tell (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_sync"
    (func $fd_sync (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_datasync"
    (func $fd_datasync (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_fdstat_get"
    (func $fd_fdstat_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_fdstat_set_flags"
    (func $fd_fdstat_set_flags (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_prestat_get"
    (func $fd_prestat_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_prestat_dir_name"
    (func $fd_prestat_dir_name (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_renumber"
    (func $fd_renumber (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_filestat_get"
    (func $fd_filestat_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_advise"
    (func $fd_advise (param i32 i64 i64 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_allocate"
    (func $fd_allocate (param i32 i64 i64) (result i32)))

  (memory (export "memory") 1)

  ;; Test fd_write to stdout
  (func (export "test_fd_write_stdout") (result i32)
    ;; iovec at offset 256: {buf=272, len=5}
    (i32.store (i32.const 256) (i32.const 272))
    (i32.store (i32.const 260) (i32.const 5))
    ;; "hello" at offset 272
    (i32.store8 (i32.const 272) (i32.const 104))
    (i32.store8 (i32.const 273) (i32.const 101))
    (i32.store8 (i32.const 274) (i32.const 108))
    (i32.store8 (i32.const 275) (i32.const 108))
    (i32.store8 (i32.const 276) (i32.const 111))
    ;; fd_write(fd=1, iovs=256, iovs_len=1, nwritten=288)
    (call $fd_write (i32.const 1) (i32.const 256) (i32.const 1) (i32.const 288))
  )

  ;; Test fd_write to stderr
  (func (export "test_fd_write_stderr") (result i32)
    (i32.store (i32.const 256) (i32.const 272))
    (i32.store (i32.const 260) (i32.const 3))
    (i32.store8 (i32.const 272) (i32.const 101))
    (i32.store8 (i32.const 273) (i32.const 114))
    (i32.store8 (i32.const 274) (i32.const 114))
    (call $fd_write (i32.const 2) (i32.const 256) (i32.const 1) (i32.const 288))
  )

  ;; Test fd_fdstat_get on stdout
  (func (export "test_fd_fdstat_get") (result i32)
    (call $fd_fdstat_get (i32.const 1) (i32.const 512))
  )

  ;; Test fd_prestat_get on fd 3 (first preopened dir)
  (func (export "test_fd_prestat_get") (result i32)
    (call $fd_prestat_get (i32.const 3) (i32.const 512))
  )

  ;; Test fd_prestat_dir_name
  (func (export "test_fd_prestat_dir_name") (result i32)
    (call $fd_prestat_dir_name (i32.const 3) (i32.const 1024) (i32.const 256))
  )

  ;; Test fd_filestat_get on stdout
  (func (export "test_fd_filestat_get") (result i32)
    (call $fd_filestat_get (i32.const 1) (i32.const 512))
  )

  ;; Test fd_sync on stdout
  (func (export "test_fd_sync") (result i32)
    (call $fd_sync (i32.const 1))
  )

  ;; Test fd_datasync on stdout
  (func (export "test_fd_datasync") (result i32)
    (call $fd_datasync (i32.const 1))
  )

  ;; Test fd_fdstat_set_flags
  (func (export "test_fd_fdstat_set_flags") (result i32)
    (call $fd_fdstat_set_flags (i32.const 1) (i32.const 0))
  )

  ;; Test fd_seek on invalid fd
  (func (export "test_fd_seek") (result i32)
    (call $fd_seek (i32.const 1) (i64.const 0) (i32.const 0) (i32.const 512))
  )

  ;; Test fd_tell on invalid fd
  (func (export "test_fd_tell") (result i32)
    (call $fd_tell (i32.const 1) (i32.const 512))
  )

  ;; Test fd_advise
  (func (export "test_fd_advise") (result i32)
    (call $fd_advise (i32.const 1) (i64.const 0) (i64.const 100) (i32.const 0))
  )
)
