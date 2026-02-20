# dustlink Documentation

This directory contains complete Markdown documentation for `dustlink`.

## Documentation Index

- `getting_started.md`: build, install, and first commands.
- `architecture.md`: component boundaries and execution model.
- `cli_reference.md`: Rust CLI contract and argument behavior.
- `backend_resolution.md`: backend discovery order and environment overrides.
- `dustlink_forge.md`: `DustLink` forge API reference.
- `error_codes.md`: linker error constants and domain error behavior.
- `linker_cli.md`: gcc/ld/lld-style flag normalization and validation profile.
- `linker_elf.md`: ELF constants and header helper APIs.
- `linker_flat.md`: flat/MBR constants and helpers.
- `linker_image.md`: image orchestration constants and APIs.
- `linker_link.md`: object/linker integration API.
- `linker_reloc.md`: relocation constants and processing APIs.
- `linker_sections.md`: section table constants and APIs.
- `linker_symbol.md`: symbol table constants and APIs.
- `testing.md`: current test modules and validation notes.

## Scope

`dustlink` currently has two implementation layers:

- A production Rust CLI frontend (`src/main.rs`) that is compatible with `ld.lld`-style invocation and forwards arguments to an external linker backend.
- Dust-language forge modules (`src/*.ds`) that now implement an internal linker MVP for format/target validation, section/symbol/relocation planning, and image checks.

The Rust CLI is still the operational entrypoint in this repository state, while the internal Dust linker path is being expanded module-by-module.
