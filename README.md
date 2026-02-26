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
  - machine/cpu selection follows resolved target architecture (`x86_64` vs `aarch64/arm64`)
  - PE compatibility-state toggles (`/NOENTRY`, `/DYNAMICBASE`, `/NXCOMPAT`, `/LARGEADDRESSAWARE`) are emitted into real PE header fields
- Script application:
  - `-T <script>` / `--script <script>`
  - directive support: `ENTRY`, `OUTPUT`, `OUTPUT_FORMAT`, `OUTPUT_ARCH`, `TARGET`, `SEARCH_DIR`, `INPUT`, `GROUP`, `AS_NEEDED`, `NO_AS_NEEDED`, `EXTERN`, `PROVIDE`, `PROVIDE_HIDDEN`, `INCLUDE`, `ASSERT`
  - compatibility blocks: `PHDRS`, `VERSION` (block-shape validated)
  - script expression coverage for `ORIGIN(...)`, `LENGTH(...)`, `ADDR(...)`, `LOADADDR(...)`, `SIZEOF(...)`, `ALIGN(...)`, plus additive/subtractive arithmetic
  - script expression coverage also includes unary, `*`, `/`, `%`, shifts, and bitwise operators
  - direct script symbol assignments (`SYMBOL = <expr>`) are supported
  - unknown script directive heads are rejected (no silent acceptance)
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
  - dynamic-unresolved policy controls: `--no-undefined`, `--error-unresolved-symbols`, `--allow-shlib-undefined`, `--no-allow-shlib-undefined`
  - dynamic-tag controls: `--enable-new-dtags` / `--disable-new-dtags`
  - transitive `DT_NEEDED` policy controls: `--copy-dt-needed-entries` / `--no-copy-dt-needed-entries`
  - compatibility-state controls: `--hash-style`, `--threads`, `--thread-count`, `--eh-frame-hdr`, `--fatal-warnings`, `--color-diagnostics`, `--print-gc-sections`, `--icf=*`
  - broader compatibility controls: `--version-script`, `--dynamic-list`, `--trace-symbol`, `--print-map`, `--start-lib`, `--end-lib`, `--emit-relocs`, `--strip-all`
  - `lld-link` compatibility controls: `/OUT:`, `/ENTRY:`, `/MACHINE:`, `/LIBPATH:`, `/DEFAULTLIB:`, `/MAP[:file]`, `/DLL`, `/SUBSYSTEM:`, `/OPT:`, `/WX`, `/NOENTRY`, `/DYNAMICBASE`, `/NXCOMPAT`, `/LARGEADDRESSAWARE`
  - shared-object symbol ingestion for exported symbol resolution across ELF, PE, COFF, and Mach-O metadata paths

## Supported Targets and Relocations

- Target IDs are architecture-aware:
  - `none`
  - `x86_64-linux`, `x86_64-windows`, `x86_64-macos`
  - `aarch64-linux`, `aarch64-windows`, `aarch64-macos`
- CLI target parsing preserves architecture for both `x86_64` and `aarch64/arm64` aliases.
- CLI target parsing also accepts musl triples, Windows GNU triples, and bare-metal `*-none[-elf]` aliases.
- ELF object validator machine coverage includes `EM_X86_64` and `EM_AARCH64`.
- relocation application is machine-aware per ingested object via runtime `object.machine`.
- COFF and Mach-O object ingestion now use refined machine-aware relocation-kind mapping during relocation record ingest.
- Core relocation set includes:
  - `R_X86_64_NONE`, `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_PLT32`
  - `R_X86_64_GLOB_DAT`, `R_X86_64_JUMP_SLOT`, `R_X86_64_RELATIVE`
  - `R_X86_64_GOTPCREL`, `R_X86_64_32`, `R_X86_64_32S`
  - `R_X86_64_GOTPCRELX`, `R_X86_64_REX_GOTPCRELX`
  - AArch64 core + instruction forms:
    - `R_AARCH64_NONE`, `R_AARCH64_ABS64`, `R_AARCH64_ABS32`, `R_AARCH64_PREL64`, `R_AARCH64_PREL32`
    - `R_AARCH64_CALL26`, `R_AARCH64_JUMP26`, `R_AARCH64_CONDBR19`, `R_AARCH64_TSTBR14`
    - `R_AARCH64_LD_PREL_LO19`, `R_AARCH64_ADR_PREL_LO21`, `R_AARCH64_ADR_PREL_PG_HI21`, `R_AARCH64_ADR_PREL_PG_HI21_NC`
    - `R_AARCH64_ADD_ABS_LO12_NC`, `R_AARCH64_LDST8/16/32/64/128_ABS_LO12_NC`
    - MOVW families: `R_AARCH64_MOVW_UABS_*`, `R_AARCH64_MOVW_SABS_*`, `R_AARCH64_MOVW_PREL_*`
  - AArch64 TLS relocation coverage:
    - `TLSGD` / `TLSLD` / `TLSDESC` instruction-form relocation IDs are recognized by ELF ingest and relocation validation
    - `R_AARCH64_TLS_DTPMOD`, `R_AARCH64_TLS_DTPREL`, `R_AARCH64_TLS_TPREL` are applied for non-shared links using host-runtime TLS layout metadata helpers
    - shared-link AArch64 TLS data reloc handling is partially differentiated: `TLS_DTPREL` resolves from TLS layout metadata, shared-link `TLS_TPREL` is rejected as invalid, and shared-link `TLS_DTPMOD` remains unsupported
    - TLSLE/TLSLD low12 offset instruction forms (`*_ADD_*_LO12_NC`, `*_LDST64_*_LO12_NC`, `*_LDST128_*_LO12_NC`) are applied in non-shared links using host-runtime TLS offset helpers
    - `R_AARCH64_TLSDESC_CALL` is applied as a validated `BLR` preserve relocation
    - remaining TLS instruction/descriptor-family relocation application (TLSGD/TLSLD/TLSDESC descriptor sequences) remains strict `ERR_NOT_IMPLEMENTED_YET` pending full TLS descriptor/GOT metadata and relaxation semantics

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
- `-z` / `-z<...>` (`relro`, `norelro`, `now`, `lazy`, `execstack`, `noexecstack`, `defs`, `undefs`, plus accepted compatibility tokens `text`, `notext`, `origin`)
- `-u`, `--undefined`, `--require-defined`
- `--no-undefined`, `--error-unresolved-symbols`
- `--allow-shlib-undefined`
- `--no-allow-shlib-undefined`
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
  - `/NOENTRY`, `/DYNAMICBASE`, `/NXCOMPAT`, `/LARGEADDRESSAWARE`
- `--help`, `--version`

Compatibility spellings for common ld/lld flags are accepted.  
Core linker-affecting paths (`--target`/`-m`, `--defsym`, `--build-id`, `-z`, required-symbol flags, sysroot/rpath/rpath-link, dynamic policy flags, group/static/shared toggles, and hash/thread/icf-related compatibility controls) are wired to internal linker state.
`lld-link` compatibility controls `/NOENTRY`, `/DYNAMICBASE`, `/NXCOMPAT`, and `/LARGEADDRESSAWARE` are now state-wired into PE writer behavior.
Additional soft-compatibility families (`--warn-*`, `--time-trace*`, `--lto-*`, and `/GUARD:*`-style slash families) are accepted to reduce false hard-fail paths during lld-profile migrations.
These compatibility no-op paths now emit diagnostics, and become hard failures when `--fatal-warnings` (or `/WX`) is enabled.

## Recent Parity Improvements

- `--dependency-file` is now state-wired and emits a depfile after successful links.
- `--emit-relocs` is now state-wired and expands map-row output with relocation rows.
- `--hash-style` now affects ELF dynamic-tag emission (`DT_HASH` / `DT_GNU_HASH`) in Dust runtime host writer output.
- `--print-gc-sections` now prints section-drop diagnostics during GC-aware alloc-section selection.
- Unsupported flag/target failures now emit explicit diagnostics instead of returning only status codes.
- AArch64 ELF relocation coverage now includes instruction bitfield patching (`ADR_PREL_LO21`, MOVW families, `LD/ST` lo12 variants, branch/literal forms) and stricter TLS-family handling:
  - `TLSDESC_CALL` now validates `BLR` instruction encoding instead of acting as a generic no-op
  - AArch64 TLS data relocs (`TLS_DTPMOD`/`TLS_DTPREL`/`TLS_TPREL`) now use host-runtime TLS layout metadata for non-shared links
  - TLS instruction/descriptor-family relocation application now fails with `ERR_NOT_IMPLEMENTED_YET` instead of silently patching placeholder values
- Shared-library ingest now propagates shared-object ingest failures directly (no Dust-side `ERR_NOT_IMPLEMENTED_YET` swallow path).
- Host shared-object ingest now treats unknown/unsupported payload formats as `ERR_INVALID_FORMAT` instead of silently succeeding.
- Host shared-object ingest now also rejects cross-target / wrong-ABI / wrong-kind shared inputs before symbol ingest (ELF requires target machine + `ET_DYN`; PE requires Windows target + DLL + matching machine; Mach-O requires macOS target + matching CPU + dylib file type).
- Host shared-object ingest now filters non-exported metadata entries during symbol ingestion (ELF hidden/internal dynsyms and Mach-O private extern/debug symbols are skipped).
- Needed-library recording now prefers embedded shared-library names when available (`DT_SONAME`, PE export DLL name, Mach-O install name) instead of filename-only normalization.

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
