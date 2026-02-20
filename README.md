# DustLink

`dustlink` is the Dust Programming Language internal linker project.

## Scope

- Platform-agnostic linker architecture (not XDV-specific).
- Dust source (`.ds`) linker pipeline.
- Host runtime implementation for object ingestion and file emission.
- Compatibility-oriented CLI surface for gcc/ld/lld workflows.

## Internal Linker Policy

`dustlink` is not a backend pass-through wrapper.  
`src/main.ds` routes directly into Dust linker modules.

## Current Capability Snapshot

- Object ingestion:
  - ELF64 relocatable objects
  - COFF x86_64 objects
  - Mach-O 64-bit x86_64 objects
- Archive ingestion:
  - `.a` and `.lib` search/ingest via `-L/-l` and `--library-path/--library`
  - deterministic search path order
- Output writers:
  - ELF executable (minimal)
  - flat binary
  - MBR image
  - PE executable (minimal)
  - Mach-O executable (minimal)
- Script application:
  - `-T <script>` / `--script <script>`
  - basic directives: `ENTRY`, `OUTPUT_FORMAT`, `SEARCH_DIR`, `INPUT`, `GROUP`

## Supported Targets and Relocations

- Target IDs: `x86_64` families (`none/linux/windows/macos` IDs).
- Core relocation set: `R_X86_64_NONE`, `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_32`, `R_X86_64_32S`.

## CLI Compatibility Surface

Primary options:

- `-o`, `--output`
- `-e`, `--entry`, `--entry-point`
- `--image-base`
- `-Ttext`
- `-T`, `--script`
- `--oformat` (`elf64`, `binary`, `mbr`, `pe`, `macho64` plus aliases)
- `-L`, `--library-path`
- `-l`, `--library`
- `-Map`, `--Map`, `--map-file`
- `-s`, `--strip-debug`
- `--gc-sections`
- `--allow-multiple-definition`
- `--start-group`, `--end-group`
- `--help`, `--version`

Accepted compatibility flags (currently consumed/no-op) include common lld flags like `--build-id`, `--threads=*`, `--target=*`, and related variants.

## Build

Check:

```bash
dust check src/
```

Build:

```bash
dust build src --out target/dust/dustlink
```

## Status

`dustlink` has advanced beyond initial ELF-only MVP behavior, but it is not yet full lld parity across every flag and script semantic.

See `changelog.md` for detailed change history.
