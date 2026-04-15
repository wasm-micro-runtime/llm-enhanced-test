(module
  ;; WASI imports for path operations
  (import "wasi_snapshot_preview1" "path_open"
    (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_create_directory"
    (func $path_create_directory (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_remove_directory"
    (func $path_remove_directory (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_unlink_file"
    (func $path_unlink_file (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_filestat_get"
    (func $path_filestat_get (param i32 i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_rename"
    (func $path_rename (param i32 i32 i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_symlink"
    (func $path_symlink (param i32 i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_readlink"
    (func $path_readlink (param i32 i32 i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write"
    (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_read"
    (func $fd_read (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close"
    (func $fd_close (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_seek"
    (func $fd_seek (param i32 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_readdir"
    (func $fd_readdir (param i32 i32 i32 i64 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_filestat_get"
    (func $fd_filestat_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_filestat_set_times"
    (func $fd_filestat_set_times (param i32 i64 i64 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_filestat_set_size"
    (func $fd_filestat_set_size (param i32 i64) (result i32)))
  (import "wasi_snapshot_preview1" "fd_pread"
    (func $fd_pread (param i32 i32 i32 i64 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_pwrite"
    (func $fd_pwrite (param i32 i32 i32 i64 i32) (result i32)))

  (memory (export "memory") 2)

  ;; Helper: store string at offset, return length
  ;; "testdir" -> 7 bytes at offset 8192
  (data (i32.const 8192) "testdir\00")
  ;; "testfile.txt" -> 12 bytes at offset 8208
  (data (i32.const 8208) "testfile.txt\00")
  ;; "newname.txt" -> 11 bytes at offset 8224
  (data (i32.const 8224) "newname.txt\00")
  ;; "Hello from WASI" -> 15 bytes at offset 8240
  (data (i32.const 8240) "Hello from WASI\00")
  ;; "symlink_target" -> 14 bytes at offset 8256
  (data (i32.const 8256) "symlink_target\00")
  ;; "symlink_name" -> 12 bytes at offset 8272
  (data (i32.const 8272) "symlink_name\00")

  ;; Test path_create_directory
  (func (export "test_path_create_directory") (result i32)
    ;; path_create_directory(fd=3, path="testdir", path_len=7)
    (call $path_create_directory (i32.const 3) (i32.const 8192) (i32.const 7))
  )

  ;; Test path_open (create + write file)
  (func (export "test_path_open_create") (result i32)
    ;; path_open(dirfd=3, dirflags=0, path="testfile.txt", path_len=12,
    ;;   oflags=CREAT(1)|TRUNC(8)=9, fs_rights_base=all, fs_rights_inheriting=all,
    ;;   fdflags=0, fd_out=512)
    (call $path_open
      (i32.const 3) (i32.const 0)
      (i32.const 8208) (i32.const 12)
      (i32.const 9)
      (i64.const 0x1FFFFFFF)
      (i64.const 0x1FFFFFFF)
      (i32.const 0) (i32.const 512))
  )

  ;; Test full file write + read cycle
  (func (export "test_file_write_read") (result i32)
    (local $fd i32)
    (local $err i32)
    ;; Open/create file
    (local.set $err
      (call $path_open
        (i32.const 3) (i32.const 0)
        (i32.const 8208) (i32.const 12)
        (i32.const 9)
        (i64.const 0x1FFFFFFF)
        (i64.const 0x1FFFFFFF)
        (i32.const 0) (i32.const 512)))
    (if (local.get $err) (then (return (local.get $err))))
    (local.set $fd (i32.load (i32.const 512)))

    ;; Write "Hello from WASI" via iovec
    ;; iovec at 256: {buf=8240, len=15}
    (i32.store (i32.const 256) (i32.const 8240))
    (i32.store (i32.const 260) (i32.const 15))
    (local.set $err
      (call $fd_write (local.get $fd) (i32.const 256) (i32.const 1) (i32.const 288)))
    (if (local.get $err) (then
      (drop (call $fd_close (local.get $fd)))
      (return (local.get $err))))

    ;; Seek back to start
    (local.set $err
      (call $fd_seek (local.get $fd) (i64.const 0) (i32.const 0) (i32.const 520)))
    (if (local.get $err) (then
      (drop (call $fd_close (local.get $fd)))
      (return (local.get $err))))

    ;; Read back
    ;; iovec at 256: {buf=4096, len=64}
    (i32.store (i32.const 256) (i32.const 4096))
    (i32.store (i32.const 260) (i32.const 64))
    (local.set $err
      (call $fd_read (local.get $fd) (i32.const 256) (i32.const 1) (i32.const 288)))

    ;; filestat_get on the opened file
    (drop (call $fd_filestat_get (local.get $fd) (i32.const 1024)))

    ;; filestat_set_times
    (drop (call $fd_filestat_set_times (local.get $fd) (i64.const 0) (i64.const 0) (i32.const 0)))

    ;; Close
    (drop (call $fd_close (local.get $fd)))
    (local.get $err)
  )

  ;; Test path_filestat_get
  (func (export "test_path_filestat_get") (result i32)
    (call $path_filestat_get (i32.const 3) (i32.const 0) (i32.const 8208) (i32.const 12) (i32.const 1024))
  )

  ;; Test fd_readdir on preopened dir
  (func (export "test_fd_readdir") (result i32)
    (call $fd_readdir (i32.const 3) (i32.const 4096) (i32.const 4096) (i64.const 0) (i32.const 512))
  )

  ;; Test fd_pwrite + fd_pread
  (func (export "test_pwrite_pread") (result i32)
    (local $fd i32)
    (local $err i32)
    ;; Open file
    (local.set $err
      (call $path_open
        (i32.const 3) (i32.const 0)
        (i32.const 8208) (i32.const 12)
        (i32.const 9)
        (i64.const 0x1FFFFFFF)
        (i64.const 0x1FFFFFFF)
        (i32.const 0) (i32.const 512)))
    (if (local.get $err) (then (return (local.get $err))))
    (local.set $fd (i32.load (i32.const 512)))

    ;; fd_pwrite: iovec at 256: {buf=8240, len=15}, offset=0
    (i32.store (i32.const 256) (i32.const 8240))
    (i32.store (i32.const 260) (i32.const 15))
    (drop (call $fd_pwrite (local.get $fd) (i32.const 256) (i32.const 1) (i64.const 0) (i32.const 288)))

    ;; fd_pread: iovec at 256: {buf=4096, len=64}, offset=0
    (i32.store (i32.const 256) (i32.const 4096))
    (i32.store (i32.const 260) (i32.const 64))
    (local.set $err
      (call $fd_pread (local.get $fd) (i32.const 256) (i32.const 1) (i64.const 0) (i32.const 288)))

    (drop (call $fd_close (local.get $fd)))
    (local.get $err)
  )

  ;; Test path_unlink_file
  (func (export "test_path_unlink") (result i32)
    (call $path_unlink_file (i32.const 3) (i32.const 8208) (i32.const 12))
  )

  ;; Test path_remove_directory
  (func (export "test_path_remove_directory") (result i32)
    (call $path_remove_directory (i32.const 3) (i32.const 8192) (i32.const 7))
  )

  ;; Test path_rename
  (func (export "test_path_rename") (result i32)
    ;; First create the file
    (drop (call $path_open
      (i32.const 3) (i32.const 0)
      (i32.const 8208) (i32.const 12)
      (i32.const 9)
      (i64.const 0x1FFFFFFF)
      (i64.const 0x1FFFFFFF)
      (i32.const 0) (i32.const 512)))
    (drop (call $fd_close (i32.load (i32.const 512))))
    ;; path_rename(old_fd=3, old_path, old_len, new_fd=3, new_path, new_len)
    (call $path_rename
      (i32.const 3) (i32.const 8208) (i32.const 12)
      (i32.const 3) (i32.const 8224) (i32.const 11))
  )
)
