# Architecture

## Workspace Layout

- `Cargo.toml`: Rust package metadata for `dustlink` binary.
- `src/main.rs`: linker frontend that resolves and executes backend linker.
- `src/dustlink.ds`: top-level Dust linker forge API.
- `src/linker_*.ds`: Dust module APIs for ELF, flat image, sections, symbols, relocations, image orchestration, and external linker integration.
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

## Dust Module Layer

The Dust modules define a linker API surface and constants by forge/domain.

Current state:

- `K` domain functions largely return `0` placeholders.
- `Q` and `Phi` domains mostly return `100` (`ERR_DOMAIN_NOT_AVAILABLE` convention), with some query-style methods returning `0`.

This means the Dust API layer currently serves as interface/spec scaffolding rather than complete executable linker logic.
