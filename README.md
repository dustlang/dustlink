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
  - deterministic search order: explicit `-L` paths, then `-rpath-link` (dynamic mode), then target/sysroot default library roots
  - dynamic-mode `-l` resolution prefers shared objects (`.so`/`.dylib`/`.dll`) before static archives
  - static-mode `-l` resolution uses static archives only
  - exact-name `-l:<file>` token support
- Output writers:
  - ELF executable (minimal)
  - flat binary
  - MBR image
  - PE executable (minimal)
  - Mach-O executable (minimal)
- Script application:
  - `-T <script>` / `--script <script>`
  - directive support: `ENTRY`, `OUTPUT`, `OUTPUT_FORMAT`, `OUTPUT_ARCH`, `TARGET`, `SEARCH_DIR`, `INPUT`, `GROUP`, `AS_NEEDED`, `NO_AS_NEEDED`, `EXTERN`, `PROVIDE`, `PROVIDE_HIDDEN`, `INCLUDE`
  - subset coverage: `MEMORY` (`ORIGIN`, `LENGTH`) and `SECTIONS` location-counter assignment (`. = <addr>`)
- Link-mode controls:
  - `--gc-sections` / `--no-gc-sections` (GC-aware alloc section selection)
  - `--allow-multiple-definition`
  - `--defsym name=value`
  - `--target=<triple>` / `-m<emulation>` target selection
  - `--sysroot`, `-rpath`/`--rpath`, and `-rpath-link`/`--rpath-link`
  - dynamic-unresolved policy controls: `--no-undefined`, `--error-unresolved-symbols`, `--allow-shlib-undefined`
  - dynamic-tag controls: `--enable-new-dtags` / `--disable-new-dtags`
  - transitive `DT_NEEDED` policy controls: `--copy-dt-needed-entries` / `--no-copy-dt-needed-entries`
  - ELF shared-object dynsym ingest for exported symbol resolution

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
- `--sysroot`
- `-rpath`, `--rpath`
- `-rpath-link`, `--rpath-link`
- `-Map`, `--Map`, `--map-file`
- `-s`, `--strip-debug`
- `--gc-sections`
- `--no-gc-sections`
- `--allow-multiple-definition`
- `--defsym` / `--defsym=<name=value>`
- `--build-id` / `--build-id=<none|fast|md5|sha1|uuid|0x...>`
- `--target` / `--target=<triple>`
- `-m` / `-m<emulation>`
- `-z` / `-z<...>`
- `-u`, `--undefined`, `--require-defined`
- `--no-undefined`, `--error-unresolved-symbols`
- `--allow-shlib-undefined`
- `--enable-new-dtags`, `--disable-new-dtags`
- `--copy-dt-needed-entries`, `--no-copy-dt-needed-entries`
- `--start-group`, `--end-group`
- `--help`, `--version`

Accepted compatibility flags still consumed as no-op/value-skip include common diagnostics/stat controls such as `--threads=*`.  
`--target`/`-m`, `--defsym`, `--build-id`, `-z`, required-symbol flags, `--sysroot`, `-rpath`, `-rpath-link`, dynamic policy flags, and group/static/shared toggles are wired into linker state.

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
