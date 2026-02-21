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
  - COFF 64-bit objects (`x86_64` and `arm64` machine IDs)
  - Mach-O 64-bit objects (`x86_64` and `arm64` CPU IDs)
- Archive ingestion:
  - `.a` and `.lib` search/ingest via `-L/-l` and `--library-path/--library`
  - deterministic search order: explicit `-L` paths, then `-rpath-link` (dynamic mode), then target/sysroot default library roots
  - dynamic-mode `-l` resolution prefers shared objects (`.so`/`.dylib`/`.dll`) before static archives
  - static-mode `-l` resolution uses static archives only
  - exact-name `-l:<file>` token support
- Output writers:
  - ELF executable
  - flat binary
  - MBR image
  - PE executable (multi-section, section-per-chunk emission)
  - Mach-O executable (multi-section segment/section emission)
- Script application:
  - `-T <script>` / `--script <script>`
  - directive support: `ENTRY`, `OUTPUT`, `OUTPUT_FORMAT`, `OUTPUT_ARCH`, `TARGET`, `SEARCH_DIR`, `INPUT`, `GROUP`, `AS_NEEDED`, `NO_AS_NEEDED`, `EXTERN`, `PROVIDE`, `PROVIDE_HIDDEN`, `INCLUDE`, `ASSERT`
  - compatibility blocks: `PHDRS`, `VERSION` (block-shape validated)
  - script expression coverage for `ORIGIN(...)`, `LENGTH(...)`, `ADDR(...)`, `LOADADDR(...)`, `SIZEOF(...)`, `ALIGN(...)`, plus additive/subtractive arithmetic
  - `SECTIONS` coverage: location-counter assignment (`. = <expr>`), output-address forms (`.text <expr> : { ... }`), and `AT(<expr>)` load-address capture
  - `SEARCH_DIR(=...)` resolves through configured `--sysroot` when present
  - `INPUT` family token handling recognizes `-L` and `-l` forms
  - block-aware script statement splitting for multi-line `MEMORY`/`SECTIONS` blocks
  - `ENTRY(symbol)` now registers required-symbol intent when symbol resolution is deferred
- Link-mode controls:
  - `--gc-sections` / `--no-gc-sections` (GC-aware alloc section selection)
  - `--allow-multiple-definition`
  - `--defsym name=value`
  - `--target=<triple>` / `-m<emulation>` target selection
  - `--sysroot`, `-rpath`/`--rpath`, and `-rpath-link`/`--rpath-link`
  - dynamic-unresolved policy controls: `--no-undefined`, `--error-unresolved-symbols`, `--allow-shlib-undefined`
  - dynamic-tag controls: `--enable-new-dtags` / `--disable-new-dtags`
  - transitive `DT_NEEDED` policy controls: `--copy-dt-needed-entries` / `--no-copy-dt-needed-entries`
  - compatibility-state controls: `--hash-style`, `--threads`, `--thread-count`, `--eh-frame-hdr`, `--fatal-warnings`, `--color-diagnostics`, `--print-gc-sections`, `--icf=*`
  - broader compatibility controls: `--version-script`, `--dynamic-list`, `--trace-symbol`, `--print-map`, `--start-lib`, `--end-lib`, `--emit-relocs`, `--strip-all`
  - `lld-link` compatibility controls: `/OUT:`, `/ENTRY:`, `/MACHINE:`, `/LIBPATH:`, `/DEFAULTLIB:`, `/MAP[:file]`, `/DLL`, `/SUBSYSTEM:`, `/OPT:`, `/WX`
  - shared-object symbol ingestion for exported symbol resolution across ELF, PE, COFF, and Mach-O metadata paths

## Supported Targets and Relocations

- Target IDs: platform families (`none/linux/windows/macos` IDs).
- CLI target parsing accepts both `x86_64` and `aarch64/arm64` triple aliases and maps them into platform target IDs.
- ELF object validator machine coverage includes `EM_X86_64` and `EM_AARCH64`.
- Core relocation set includes:
  - `R_X86_64_NONE`, `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_PLT32`
  - `R_X86_64_GLOB_DAT`, `R_X86_64_JUMP_SLOT`, `R_X86_64_RELATIVE`
  - `R_X86_64_GOTPCREL`, `R_X86_64_32`, `R_X86_64_32S`
  - `R_X86_64_GOTPCRELX`, `R_X86_64_REX_GOTPCRELX`

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
- `--version-script`, `--dynamic-list`, `--trace-symbol`
- `--print-map`, `--start-lib`, `--end-lib`
- `--emit-relocs`, `--strip-all`
- `lld-link` compatibility spellings:
  - `/OUT:<path>`, `/ENTRY:<symbol|addr>`, `/MACHINE:<arch>`
  - `/LIBPATH:<dir>`, `/DEFAULTLIB:<name>`, `/MAP` or `/MAP:<path>`
  - `/DLL`, `/SUBSYSTEM:<kind>`, `/OPT:<token>`, `/WX`, `/WX:NO`
- `--help`, `--version`

Compatibility spellings for common ld/lld flags are accepted.  
Core linker-affecting paths (`--target`/`-m`, `--defsym`, `--build-id`, `-z`, required-symbol flags, sysroot/rpath/rpath-link, dynamic policy flags, group/static/shared toggles, and hash/thread/icf-related compatibility controls) are wired to internal linker state.

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

`dustlink` has advanced beyond initial ELF-only behavior, but it is not yet full lld parity across every flag/script/cross-format semantic.

See `changelog.md` for detailed change history.
