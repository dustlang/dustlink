# Getting Started

## Prerequisites

- Rust toolchain with `cargo`.
- One linker backend available:
  - `ld.lld` in `PATH`, or
  - `rust-lld` in `PATH`, or
  - `rust-lld` in Rust sysroot.

## Build

From `dustlink/`:

```bash
cargo build --release
```

Binary output (Windows):

```text
target\release\dustlink.exe
```

## Basic Usage

Display help:

```bash
dustlink --help
```

Show version:

```bash
dustlink --version
```

Print the resolved backend path:

```bash
dustlink --print-backend
```

Pass normal linker flags through to backend:

```bash
dustlink -o kernel.bin --entry 0x100000 obj1.o obj2.o
```

## Setting an Explicit Backend

```bash
# command name looked up in PATH
set DUSTLINK_BACKEND=ld.lld

# or absolute path
set DUSTLINK_BACKEND=C:\LLVM\bin\ld.lld.exe
```

If `DUSTLINK_BACKEND` is set and cannot be resolved, `dustlink` exits with an error.
