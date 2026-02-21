# dustlink Documentation

This directory contains complete Markdown documentation for `dustlink`.

## Documentation Index

- `getting_started.md`: build, install, and first commands.
- `architecture.md`: component boundaries and execution model.
- `cli_reference.md`: Dust-native CLI contract and argument behavior.
- `backend_resolution.md`: compiler linker selection behavior for host builds.
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
- `../changelog.md`: project change history.

## Scope

`dustlink` currently has two active tracks:

- Dust-language forge modules (`src/*.ds`) implementing internal linker behavior for CLI parsing, object/archive ingestion, symbol/relocation resolution, and image writing.
- Dust-native entrypoint (`src/main.ds`) that routes into internal Dust linker functions.

`dustlink` is no longer documented as a host-backend wrapper.

## Current Implementation Highlights

- Object formats: ELF64, COFF x86_64, Mach-O 64-bit x86_64.
- Output formats: ELF, flat, MBR, PE, Mach-O.
- Archive/library resolution:
  - deterministic `-L` search order
  - dynamic-mode `-l` search prefers shared objects before static archives
  - static-mode `-l` search prefers static archives
  - exact-name `-l:<file>` token support
- Linker script support: directives (`ENTRY`, `OUTPUT`, `OUTPUT_FORMAT`, `OUTPUT_ARCH`, `TARGET`, `SEARCH_DIR`, `INPUT`, `GROUP`, `AS_NEEDED`, `NO_AS_NEEDED`, `EXTERN`, `PROVIDE`, `INCLUDE`) plus `MEMORY`/`SECTIONS` subset handling.
- Dynamic-link policy controls: `--no-undefined`, `--error-unresolved-symbols`, `--allow-shlib-undefined`.
