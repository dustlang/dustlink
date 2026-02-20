# DustLink

`dustlink` is the Dust Programming Language linker project.

## Scope

- Platform-agnostic linker architecture (not XDV-specific).
- ELF/System-V-oriented MVP core in Dust source (`.ds`).
- Compatibility flag profile for gcc/ld/lld-style invocation semantics.

## Current Implementation Split

- `src/*.ds`: internal linker MVP logic written in Dust.
- `src/main.rs`: current production CLI wrapper path used by existing build pipelines.

The active migration direction is wrapper mode to internal Dust linker execution.

## Dust MVP Status

Implemented in Dust modules:

- CLI flag normalization and validation (`linker_cli.ds`)
- Error model (`linker_errors.ds`)
- ELF identity/type/machine checks (`linker_elf.ds`)
- Section/layout helpers (`linker_sections.ds`)
- Symbol rules (`linker_symbol.ds`)
- Relocation validation/math (`linker_reloc.ds`)
- Image planning/validation (`linker_image.ds`)
- Link stage target/format validation (`linker_link.ds`)
- Top-level orchestrator (`dustlink.ds`)

## Supported MVP Targets and Formats

- Targets: `x86_64` profile families (none/linux/windows/macos target IDs).
- Output formats: ELF64, flat binary, MBR image IDs.
- Relocations: `R_X86_64_NONE`, `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_32`, `R_X86_64_32S`.

## Compatibility Flags (MVP Canonical Surface)

- Output: `-o`, `--output`
- Entry: `-e`, `--entry`
- Map: `-Map`
- Image base: `--image-base`
- Text address: `-Ttext`
- Output format: `--oformat`
- Library path/library: `-L`, `-l`
- Strip: `-s`, `--strip-debug`
- Garbage collect sections: `--gc-sections`
- Multiple definition policy: `--allow-multiple-definition`
- Groups: `--start-group`, `--end-group`
- Utility: `--help`, `--version`

## Domain Policy

- `K`: active MVP behavior.
- `Q` and `Phi`: domain-unavailable returns (`100`) for operational procedures.

## Build Check

```bash
dust check src/
```
