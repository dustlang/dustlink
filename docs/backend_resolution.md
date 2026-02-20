# Backend Resolution

Source: compiler host link path in `dust/crates/dust_codegen/src/lib.rs`

## Policy

`dustlink` is not a backend pass-through wrapper. Resolution here describes how the Dust compiler links host executables.

## Link Attempt Order

General host executable build:

1. `dustlink` (preferred host linker frontend)
2. compiler driver with `-fuse-ld=lld`
3. `rust-lld`
4. `ld.lld`
5. compiler driver default linker

Bootstrap build of `dustlink` executable itself:

1. compiler driver with `-fuse-ld=lld`
2. `rust-lld`
3. `ld.lld` (platform-dependent)
4. compiler driver default linker

This avoids recursive self-invocation while producing the first Dust-built `dustlink` binary.
