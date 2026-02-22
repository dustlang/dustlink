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

- Object formats: ELF64, COFF64 (`x86_64` + `arm64` machine IDs), Mach-O 64-bit (`x86_64` + `arm64` CPU IDs).
- Output formats: ELF, flat, MBR, PE, Mach-O.
- Output writers now stamp architecture-correct headers for both `x86_64` and `aarch64/arm64` targets (ELF machine, PE machine/characteristics, Mach-O CPU type).
- PE/Mach-O writer paths now emit sectionized images from real alloc chunks instead of single synthetic payload sections.
- Archive/library resolution:
  - deterministic `-L` + `-rpath-link` + target/sysroot default search order
  - dynamic-mode `-l` search prefers shared objects before static archives
  - static-mode `-l` search prefers static archives
  - exact-name `-l:<file>` token support
- Dynamic loader/search state:
  - `--sysroot` backed default search roots
  - `-rpath` / `--rpath` emitted into ELF dynamic tags
  - `-rpath-link` / `--rpath-link` dynamic resolution paths
  - `--enable-new-dtags` / `--disable-new-dtags` (`DT_RUNPATH` vs `DT_RPATH`)
  - `--copy-dt-needed-entries` / `--no-copy-dt-needed-entries`
- Shared-symbol ingestion:
  - ELF dynsym (`.so`) exports
  - PE export table names
  - COFF external definitions
  - Mach-O external definitions
- Linker script support: directives (`ENTRY`, `OUTPUT`, `OUTPUT_FORMAT`, `OUTPUT_ARCH`, `TARGET`, `SEARCH_DIR`, `INPUT`, `GROUP`, `AS_NEEDED`, `NO_AS_NEEDED`, `EXTERN`, `PROVIDE`, `INCLUDE`, `ASSERT`) plus `PHDRS`/`VERSION` compatibility blocks with block-shape validation.
- Script parser now uses block-aware statement splitting, supports expression evaluation (`ORIGIN/LENGTH/ADDR/LOADADDR/SIZEOF/ALIGN` + `+/-`), supports `SECTIONS` output-address and `AT(...)` forms, resolves `SEARCH_DIR(=...)` against `--sysroot`, and recognizes `INPUT` token forms `-L` / `-l`.
- Script parser now also supports direct symbol assignments (`SYMBOL = <expr>`), rejects unknown directive heads, and evaluates unary/multiplicative/shift/bitwise operators.
- Dynamic-link policy controls: `--no-undefined`, `--error-unresolved-symbols`, `--allow-shlib-undefined`, `--no-allow-shlib-undefined`.
- `-z` token coverage includes relro/now/execstack families plus `defs`/`undefs` and accepted compatibility tokens `text`/`notext`/`origin`.
- Compatibility-state controls: hash-style/thread settings, eh-frame-header toggle, diagnostics toggles, print-gc toggle, and `--icf=*` mode.
- Compatibility no-op families now emit diagnostics and can hard-fail under `--fatal-warnings` / `/WX`.
- `--dependency-file` and `--emit-relocs` are state-wired (depfile output + relocation map-row reporting).
- ELF writer now consumes `--hash-style` state for dynamic hash-tag emission and host runtime consumes `--print-gc-sections` for GC drop diagnostics.
- `lld-link` spellings: `/OUT`, `/ENTRY`, `/MACHINE`, `/LIBPATH`, `/DEFAULTLIB`, `/MAP`, `/DLL`, `/SUBSYSTEM`, `/OPT`, `/WX`, `/NOENTRY`, `/DYNAMICBASE`, `/NXCOMPAT`, `/LARGEADDRESSAWARE`.
- soft-compatibility flag-family acceptance for broader ld/lld/lld-link compatibility while preserving deterministic internal link behavior.
