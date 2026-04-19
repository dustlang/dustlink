# DustLink Host Shim Design

**Version:** 0.1
**Date:** 2026-04-18
**Status:** Draft - requires dustc v0.2 for full features

## Overview

The host shim provides the DustLink linker with access to host system functionality (file I/O, memory mapping, environment variables, process execution) through a pure Dust API layer.

## Files

- `src/shim_host_v02.ds` - Current v0.1-compatible implementation
- `src/shim_host.ds` - Extended version with documentation (has parse issues with v0.1 compiler)

## Architecture

### Layer 0: Host FFI Layer

Provides basic OS/system access:

| Category | Procedures | Purpose |
|----------|------------|---------|
| **CLI Args** | `host_args_count`, `host_arg_at`, `host_args_get`, `host_args_sizes_get` | Command-line argument access |
| **Environment** | `host_env_get`, `host_env_set` | Environment variable read/write |
| **Path** | `host_path_exists`, `host_path_is_file` | Filesystem path queries |
| **File I/O** | `host_file_open`, `host_file_close`, `host_file_read`, `host_file_write`, `host_file_seek`, `host_file_tell` | File operations |
| **Memory Mapping** | `host_mmap`, `host_munmap` | Memory-mapped file I/O |
| **Logging** | `host_write_stdout`, `host_write_stderr` | Standard output/error |
| **Memory** | `host_alloc`, `host_free` | Dynamic memory allocation |

### Layer 1: Linker Operations

Provides linker-specific functionality:

| Category | Count | Purpose |
|----------|-------|---------|
| **Object Management** | 7 | Object file lifecycle |
| **Symbol Operations** | 11 | Symbol table queries |
| **Relocation** | 9 | Relocation processing |
| **Section Operations** | 18 | Section management |
| **Archive Operations** | 8 | Archive member access |
| **Output Generation** | 9 | Linked output emission |

## Constants

### File Open Flags
```dust
const O_RDONLY: UInt32 = 0;
const O_RDWR: UInt32 = 2;
const O_CREAT: UInt32 = 64;
const O_TRUNC: UInt32 = 512;
const O_WRONLY: UInt32 = 1;
```

### Memory Protection
```dust
const PROT_READ: UInt32 = 1;
const PROT_WRITE: UInt32 = 2;
const PROT_EXEC: UInt32 = 4;
```

### Memory Mapping
```dust
const MAP_PRIVATE: UInt32 = 2;
const MAP_ANONYMOUS: UInt32 = 32;
```

### Seek Whence
```dust
const SEEK_SET: UInt32 = 0;
const SEEK_CUR: UInt32 = 1;
const SEEK_END: UInt32 = 2;
```

## v0.2 Enhancements (Planned)

When dustc v0.2 is available, the following features will be added:

### Shape Types
```dust
shape FileHandle {
    fd: Int32,
    path: UInt64,
    mode: UInt32,
    valid: Bool
}

shape MappedRegion {
    base_addr: UInt64,
    length: UInt64,
    offset: Int64,
    prot: UInt32,
    valid: Bool
}

shape HostError {
    code: Int32,
    message: UInt64,
    context: UInt64,
    recoverable: Bool
}
```

### Effect Declarations
```dust
observe host_read;
observe host_write;
observe host_file_open;
observe host_file_close;
emit host_path_check;
emit host_process_spawn;
seal host_linker_build;
```

### Binding Contracts
```dust
bind host_file_open -> host_file_read contract {
    valid_handle = true,
    sequential = true
}

bind host_mmap -> host_munmap contract {
    region_valid = true,
    bounds_checked = true
}
```

## Implementation Notes

1. **Current State**: All procedures return stub values (0 or void). Actual implementation requires binding to host FFI.

2. **Parameter Naming**: `bind` is a reserved keyword in Dust v0.1/v0.2. Parameters using `bind` should be renamed to `sym_bind` or similar.

3. **Compilation**: Run `dust dir dustlink/src/shim_host_v02.ds` to validate syntax.

4. **Integration**: The shim is consumed by other linker modules (linker_fs.ds, dustlink.ds) through procedure calls.

## Migration Path

1. **Phase 1** (Current): Establish API surface in Dust
2. **Phase 2**: Implement host FFI bindings (C interop or custom runtime)
3. **Phase 3**: Add v0.2 type system, effects, and contracts
4. **Phase 4**: Full bootstrap self-compilation

## References

- Dust v0.2 Specification: https://github.com/dustlang/dustlang
- DIR Spec: `spec/12-dir.md`
- Effects: `spec/06-effects.md`
- Binding Contracts: `spec/10-binding-contracts.md`