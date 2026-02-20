# Architecture

## Workspace Layout

- `Cargo.toml`: Rust package metadata for `dustlink` binary.
- `src/main.rs`: current linker frontend that resolves and executes backend linker.
- `src/dustlink.ds`: top-level Dust linker forge API.
- `src/linker_*.ds`: Dust module APIs for CLI normalization, ELF, flat image, sections, symbols, relocations, image orchestration, and link-stage planning.
- `src/*_tests.ds`: Dust test harness modules.
- `State.toml`: Dust workspace/sector metadata.

## Runtime Flow (Rust CLI)

1. Parse argv.
2. Handle `--help` / `--version`.
3. Resolve backend (`DUSTLINK_BACKEND`, `ld.lld`, `rust-lld`, sysroot fallback).
4. Handle `--print-backend` (print and exit).
5. If backend is `rust-lld` and no flavor is provided, inject `-flavor gnu`.
6. Forward all remaining args to backend process.
7. Exit with backend exit code.

## Dust Module Layer (Internal MVP)

The Dust modules now provide deterministic linker MVP behavior for:

- flag normalization and validation (gcc/ld/lld profile)
- target and format validation
- section and image planning helpers
- symbol and relocation validation helpers
- top-level orchestrated status/error flow in `DustLink::K`

`Q` and `Phi` remain domain-unavailable for operational paths and return `100`.

## Migration Direction

`dustlink` is moving from wrapper-forwarding mode to a fully internal Dust linker.  
The Rust CLI remains the current executable entrypoint while the internal Dust linker path is expanded and validated.
