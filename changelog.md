# dustlink Changelog

## 2026-02-22

### Added

- Architecture-aware target IDs across Dust linker modules and host runtime state:
  - `none`
  - `x86_64-linux`, `x86_64-windows`, `x86_64-macos`
  - `aarch64-linux`, `aarch64-windows`, `aarch64-macos`
- CLI target/machine alias expansion now preserves ARM64/AArch64 architecture in:
  - `--target=<triple>` and `-m<emulation>`
  - `/MACHINE:ARM64`
  - script `OUTPUT_ARCH(...)` target mapping for AArch64 Linux.
- Machine-aware ELF relocation ingest validation for `EM_X86_64` and `EM_AARCH64`.
- Baseline AArch64 relocation handling in Dust relocation pipeline:
  - `R_AARCH64_NONE`
  - `R_AARCH64_ABS64`
  - `R_AARCH64_ABS32`
  - `R_AARCH64_PREL32`
- Host runtime linker intrinsic:
  - `host_linker_object_machine`
- PE compatibility-state intrinsics and state wiring:
  - `/NOENTRY`
  - `/DYNAMICBASE`
  - `/NXCOMPAT`
  - `/LARGEADDRESSAWARE`
- Additional compatibility acceptance families to reduce strict hard-fail behavior for common lld/lld-link options:
  - `--warn-*`, `--error-limit=*`, `--reproduce=*`
  - `--time-trace*`, `--lto-*`
  - `--undefined-glob=*`, `--shuffle-sections=*`
  - `/GUARD:*`, `/TIMESTAMP:*`, `/ORDER:*`, `/MERGE:*`, `/SECTION:*`, `/ALIGN:*`, `/CETCOMPAT`, `/BREPRO`
- New/expanded Dust tests for:
  - AArch64 target and machine mapping
  - AArch64 `OUTPUT_ARCH(...)` script mapping
  - soft compatibility flag-family detection
  - PE compatibility-state toggles.
- Additional host-CLI alias coverage for:
  - musl target triples (`x86_64-*linux-musl`, `aarch64-*linux-musl`)
  - Windows GNU triples (`x86_64-pc-windows-gnu`, `aarch64-pc-windows-gnu`)
  - bare-metal aliases (`*-unknown-none-elf`, `*-pc-none-elf`)
- Host runtime linker intrinsics/state wiring for:
  - `host_linker_get_fatal_warnings`
  - `host_linker_get_color_diagnostics`
  - `host_linker_get_print_gc_sections`
  - `host_linker_set_dependency_file` / `host_linker_get_dependency_file` / `host_linker_write_dependency_file`
  - `host_linker_set_emit_relocs` / `host_linker_get_emit_relocs`
- Linker script semantic additions:
  - direct symbol assignment statements (`SYMBOL = <expr>`)
  - strict rejection for unknown directive heads
  - expanded expression operators (unary, multiply/divide/mod, shifts, bitwise)
- Additional Dust tests for:
  - musl/Windows GNU/bare-metal target alias parsing
  - script symbol assignment
  - script bitwise/shift expression evaluation
  - script unknown-directive rejection
- AArch64 ELF relocation parity expansions in Dust linker modules:
  - instruction-form bitfield patching for `CALL26`, `JUMP26`, `CONDBR19`, `TSTBR14`, `LD_PREL_LO19`, `ADR_PREL_LO21`, `ADR_PREL_PG_HI21(_NC)`, `ADD_ABS_LO12_NC`, and `LDST*_ABS_LO12_NC` (including `LDST128`)
  - MOVW relocation families (`UABS`, `SABS`, `PREL`)
  - AArch64 TLS relocation ID recognition/validation for starter TLS families (`TLSGD`, `TLSLD`, `TLSDESC` instruction forms) and AArch64 TLS data relocs (`TLS_DTPMOD`, `TLS_DTPREL`, `TLS_TPREL`)
  - host-runtime-backed AArch64 TLS data relocation value computation for non-shared links (`TLS_DTPMOD`, `TLS_DTPREL`, `TLS_TPREL`) using synthesized TLS section layout metadata
  - AArch64 TLSLE/TLSLD low12 offset instruction relocation application (`ADD`/`LDST64`/`LDST128`) via host-runtime TLS offset helper reuse in non-shared links
  - `R_AARCH64_TLSDESC_CALL` application support (validated `BLR` preserve relocation) while leaving broader TLSDESC descriptor-sequence relocs explicitly unsupported
  - partial shared-link AArch64 TLS data-reloc differentiation (`TLS_DTPREL` supported via TLS layout metadata, shared-link `TLS_TPREL` invalid, shared-link `TLS_DTPMOD` still unsupported)
- Additional Dust relocation/math tests for AArch64:
  - `ADR_PREL_LO21` patching
  - MOVW patching and overflow validation
  - AArch64 TLS data relocation value validation
  - `TLSDESC_CALL` instruction-shape validation (`BLR` accepted, non-`BLR` rejected)
  - TLSLE/TLSLD low12 offset relocation-kind mapping helper coverage

### Changed

- Output writers now emit architecture-correct format headers from resolved target:
  - ELF `e_machine` set per target (`x86_64` vs `aarch64`)
  - PE machine field set per target (`x86_64` vs `arm64`)
  - Mach-O `cpu_type/cpu_subtype` set per target.
- ELF default dynamic-linker path now resolves to `/lib/ld-linux-aarch64.so.1` for AArch64 Linux targets.
- PE writer characteristics and entry behavior now follow compatibility-state toggles:
  - `/NOENTRY` controls `AddressOfEntryPoint`
  - `/DYNAMICBASE` and `/NXCOMPAT` control PE DLL characteristics
  - `/LARGEADDRESSAWARE` controls COFF characteristics.
- Compatibility no-op flag families are no longer silently consumed:
  - they now emit diagnostics
  - they become hard errors when `--fatal-warnings` or `/WX` is enabled
- `--dependency-file` now emits a depfile after successful links instead of being parse-only compatibility handling.
- `--emit-relocs` now affects map-row output by including relocation rows when enabled.
- `--hash-style` now affects ELF writer dynamic-tag emission (`DT_HASH`, `DT_GNU_HASH`).
- `--print-gc-sections` now emits drop diagnostics during GC-based section pruning.
- COFF and Mach-O object ingest relocation mapping is now machine-aware/refined instead of coarse fallback-only classification.
- ELF relocation ingest validation and linker relocation pipeline now accept/process a broader AArch64 relocation surface (including MOVW and TLS starter forms) instead of rejecting them as unsupported.
- Shared-library ingest in `linker_archive.ds` now propagates shared-object ingest errors directly instead of swallowing `ERR_NOT_IMPLEMENTED_YET`.
- Host shared-object ingest now returns `ERR_INVALID_FORMAT` for unknown/unsupported payloads instead of silently succeeding.
- Host shared-object ingest now enforces target/ABI/file-kind validation before symbol ingest (ELF `ET_DYN`, Windows PE DLL/COFF machine, Mach-O dylib CPU).
- Host shared-object ingest now filters non-exported ELF/Mach-O metadata symbols (ELF hidden/internal dynsyms, Mach-O private extern/debug entries).
- Host needed-library recording now prefers embedded shared-library names (`DT_SONAME`, PE export DLL name, Mach-O install name) when present.

### Fixed

- Unsupported flag/target failures now print explicit diagnostics instead of returning a status code with no CLI context.
- Linker script parser no longer silently accepts unknown directive heads.
- Linker script `OUTPUT_FORMAT`, `TARGET`, and `OUTPUT_ARCH` invalid values now fail with explicit invalid/unsupported statuses instead of silently succeeding.
- AArch64 64-bit TLS data relocations no longer fall through 32-bit relocation validation constraints in the Dust relocation validator.
- AArch64 TLS instruction/descriptor-family relocation application no longer silently patches placeholder values; it now returns `ERR_NOT_IMPLEMENTED_YET` until full TLS descriptor/GOT metadata and relaxation semantics are exposed to the Dust relocation pipeline.
- AArch64 TLS data relocations (`TLS_DTPMOD`/`TLS_DTPREL`/`TLS_TPREL`) no longer use placeholder relocation math in non-shared links; they now use host-runtime TLS layout metadata.
- AArch64 TLSLE/TLSLD low12 offset instruction relocations no longer fall through the blanket TLS-family `ERR_NOT_IMPLEMENTED_YET` path in non-shared links.
- `R_AARCH64_TLSDESC_CALL` no longer falls through the blanket TLS-family `ERR_NOT_IMPLEMENTED_YET` path; it now applies as a validated preserve relocation.
- AArch64 shared-link TLS data relocs no longer all fail as `ERR_NOT_IMPLEMENTED_YET`; `TLS_DTPREL` now resolves while shared-link `TLS_TPREL` fails as invalid relocation.
- `R_AARCH64_TLSDESC_CALL` patch helper no longer accepts arbitrary non-zero instructions; it now validates `BLR`-class encoding.

## 2026-02-21

### Added

- Host runtime linker intrinsics:
  - `host_linker_set_gc_sections` / `host_linker_get_gc_sections`
  - `host_linker_set_allow_multiple_definition` / `host_linker_get_allow_multiple_definition`
  - `host_linker_apply_defsym`
- Host runtime linker intrinsics for required-symbol and test state control:
  - `host_linker_require_symbol`
  - `host_linker_check_required_symbols`
  - `host_linker_required_symbol_count`
  - `host_linker_reset_state`
- `--defsym` application path (including `--defsym=name=value`) to absolute symbol definitions.
- `--target` / `-m` resolution to x86_64 linux/windows/macos target IDs.
- Target-based output-format defaults when `--oformat` is not explicitly provided.
- `-u` / `--undefined` / `--require-defined` CLI support mapped to required-symbol enforcement.
- Script semantics block:
  - `OUTPUT(...)`
  - `TARGET(...)`
  - `EXTERN(...)`
  - `PROVIDE(...)` and `PROVIDE_HIDDEN(...)`
  - `INCLUDE(...)` with bounded recursive include depth.
- New Dust test modules:
  - `src/linker_script_semantics_tests.ds`
  - `src/linker_buildid_z_tests.ds`
  - expanded CLI and cross-format behavior checks in existing test modules.
- Host runtime linker intrinsics for `--build-id` and `-z` semantics:
  - `host_linker_enable_build_id_default`
  - `host_linker_set_build_id`
  - `host_linker_get_build_id_mode`
  - `host_linker_set_z_option`
  - `host_linker_get_z_flags`
- Linker script subset coverage additions:
  - `MEMORY` keyword extraction for `ORIGIN` and `LENGTH`
  - `SECTIONS` location-counter assignment parsing (`. = <addr>`)
  - location-counter parse now works when assignment appears inside `SECTIONS { ... }` statements.
- CLI semantic coverage additions:
  - `--build-id[=<none|fast|md5|sha1|uuid|0x...>]`
  - `-z <relro|norelro|now|lazy|execstack|noexecstack>` and `-z<...>`
  - explicit test coverage for hex build-id mode, bare `--build-id` optional-value behavior, and invalid `-z` rejection.
- Shared-library parity additions:
  - host runtime shared-object ingest path: `host_linker_ingest_shared_object`
  - ELF `.so` dynsym-based exported-symbol ingestion for dynamic resolution
  - host runtime unresolved-policy intrinsics:
    - `host_linker_set_no_undefined`
    - `host_linker_get_no_undefined`
    - `host_linker_allow_dynamic_unresolved`
- Cross-format shared-symbol ingestion additions:
  - PE export table parsing for `.dll`-style shared symbol discovery
  - COFF external-definition symbol ingest path for shared symbol discovery
  - Mach-O external-definition symbol ingest path for shared symbol discovery (including underscore alias normalization)
- Search-path and runtime loader parity additions:
  - host runtime search-path intrinsics:
    - `host_linker_set_sysroot` / `host_linker_get_sysroot`
    - `host_linker_add_rpath` / `host_linker_rpath_count` / `host_linker_get_rpath`
    - `host_linker_add_rpath_link` / `host_linker_rpath_link_count` / `host_linker_get_rpath_link`
    - `host_linker_default_search_path_count` / `host_linker_get_default_search_path`
  - CLI state wiring for `--sysroot`, `-rpath`/`--rpath`, `-rpath-link`/`--rpath-link`
  - ELF dynamic runpath emission support
- Dynamic-tag and transitive-needed parity additions:
  - host runtime intrinsics:
    - `host_linker_set_new_dtags` / `host_linker_get_new_dtags`
    - `host_linker_set_copy_dt_needed_entries` / `host_linker_get_copy_dt_needed_entries`
  - CLI state wiring for `--enable-new-dtags` / `--disable-new-dtags`
  - CLI state wiring for `--copy-dt-needed-entries` / `--no-copy-dt-needed-entries`
  - ELF dynamic tag mode support (`DT_RUNPATH` or `DT_RPATH`)
  - ELF transitive `DT_NEEDED` parse/append path when copy-needed mode is enabled
- Script subset additions:
  - `OUTPUT_ARCH(...)`
  - `AS_NEEDED(...)` and `NO_AS_NEEDED(...)` scoped token ingestion
- Additional tests:
  - dynamic unresolved-policy state tests in `src/linker_buildid_z_tests.ds`
  - `OUTPUT_ARCH` and `AS_NEEDED` script semantics tests in `src/linker_script_semantics_tests.ds`
  - `--allow-shlib-undefined` CLI-value behavior test in `src/linker_host_cli_tests.ds`
  - search-path state tests for sysroot/rpath/rpath-link in `src/linker_buildid_z_tests.ds`
  - dynamic tag mode and copy-needed toggle tests in `src/linker_buildid_z_tests.ds`
  - CLI requires-value/no-value coverage for sysroot/rpath/rpath-link/new-dtags/copy-needed flags in `src/linker_host_cli_tests.ds`
  - linker-script semantics tests for:
    - `SECTIONS` output-address form (`.text 0x... : { ... }`)
    - `ENTRY(symbol)` required-symbol registration
    - multi-line `MEMORY`/`SECTIONS` block parsing
- ELF/relocation parity additions in Dust modules:
  - `EM_AARCH64` acceptance in ELF machine validator path
  - relocation ID support additions: `PLT32`, `GLOB_DAT`, `JUMP_SLOT`, `RELATIVE`, `GOTPCREL`, `GOTPCRELX`, `REX_GOTPCRELX`
- Additional compatibility-state intrinsics:
  - `host_linker_set_hash_style` / `host_linker_get_hash_style`
  - `host_linker_set_thread_count` / `host_linker_get_thread_count`
  - `host_linker_set_eh_frame_hdr`
  - `host_linker_set_fatal_warnings`
  - `host_linker_set_color_diagnostics`
  - `host_linker_set_print_gc_sections`
  - `host_linker_set_icf_mode` / `host_linker_get_icf_mode`
- Script semantic additions:
  - `SEARCH_DIR(=...)` resolves against `--sysroot` when present
  - `INPUT`/`GROUP`/`AS_NEEDED` token handling recognizes `-L` and `-l` tokens directly
  - script argument tokenizer upgraded to quote/paren-aware token splitting
- Test additions:
  - script semantic checks for sysroot-backed `SEARCH_DIR(=...)`
  - script semantic checks for `INPUT(-L...)` search-path updates
  - script semantic checks for `INPUT(-l...)` needed-library recording
  - host CLI checks for inline/value handling of `--hash-style`, `--threads`, and `--thread-count`
- Extended CLI parity wiring/compat coverage:
  - `--version-script` / `--dynamic-list` now route through script application semantics
  - `--trace-symbol` now maps to required-symbol registration
  - compatibility handling for `--print-map`, `--start-lib`, `--end-lib`, `--emit-relocs`, `--strip-all`,
    `--dependency-file`, `--compress-debug-sections`, and `--pack-dyn-relocs`
- Script semantic coverage additions:
  - `ASSERT(...)` evaluation with deterministic non-zero/zero condition handling
  - compatibility-accepted `PHDRS` and `VERSION` blocks
  - expression evaluation for `ORIGIN/LENGTH/ADDR/LOADADDR/SIZEOF/ALIGN` and `+`/`-` arithmetic
  - `SECTIONS ... AT(<expr>)` load-address capture
- Output writer parity improvements:
  - PE writer now emits sectionized images from alloc chunks (multi-section headers/materialization)
  - Mach-O writer now emits sectionized segment/section metadata from alloc chunks
- `lld-link` compatibility additions in Dust CLI parsing:
  - slash-option support for `/OUT:`, `/ENTRY:`, `/MACHINE:`, `/LIBPATH:`, `/DEFAULTLIB:`, `/MAP`/`/MAP:<file>`, `/DLL`, `/SUBSYSTEM:`, `/OPT:`, and `/WX`
  - compatibility acceptance for common slash metadata families (`/PDB:`, `/IMPLIB:`, `/MANIFEST:`, `/EXPORT:`, `/NODEFAULTLIB[:...]`, `/INCLUDE:`)
  - additional long-inline compatibility acceptance for `--plugin-opt=*`, `--mllvm=*`, and `--thinlto-*`
- Target alias expansion:
  - accepted `aarch64` / `arm64` triple/emulation aliases in both Dust CLI and host script target parsing
  - accepted COFF ARM64 and Mach-O ARM64 machine/cpu identifiers in object-format probe and shared-symbol ingest paths
- Script semantic strictness additions:
  - `PHDRS` and `VERSION` compatibility blocks now require valid block structure
  - added script tests for malformed `PHDRS`/`VERSION` rejection
- Relocation behavior additions:
  - unresolved dynamic-placeholder relocations (`PLT/GOT/JUMP_SLOT/GLOB_DAT` families) now allow zero-placeholder patching when dynamic unresolved policy permits
  - `R_X86_64_RELATIVE` now resolves via `image_base + addend`
- Additional dynamic-link parity additions:
  - `-z defs` / `-z undefs` now toggle unresolved-symbol strictness (`no_undefined`) through host runtime state
  - accepted compatibility `-z` spellings: `text`, `notext`, `origin`
  - CLI support for `--no-allow-shlib-undefined` (strict unresolved-symbol alias)
- Additional `lld-link` compatibility acceptance:
  - no-value forms `/NOENTRY`, `/DYNAMICBASE`, `/NXCOMPAT`, `/LARGEADDRESSAWARE`

### Changed

- `--gc-sections` is no longer a no-op; output segment construction now applies GC-aware section retention.
- `--allow-multiple-definition` now affects global symbol selection conflict behavior.
- Link image planning now uses dynamic calculated image size instead of fixed placeholder section-size constants.
- Script `SEARCH_DIR` and `INPUT`/`GROUP` path tokens now resolve relative to the including script directory.
- Symbol resolution checks now enforce required symbol declarations after global resolution.
- `-l`/`--library` resolution now differentiates dynamic and static mode:
  - dynamic mode prefers shared libraries (`.so`, `.dylib`, `.dll`) before static archives
  - static mode resolves static archives
  - exact-name `-l:<file>` search tokens are handled
- library resolution now includes deterministic fallback stages:
  - explicit `-L` paths first
  - `-rpath-link` paths (dynamic mode)
  - target/sysroot default search roots
- `--no-undefined` / `--error-unresolved-symbols` and `--allow-shlib-undefined` now drive unresolved-symbol policy state instead of being parse-only.
- ELF writer now includes build-id note payload when configured, applies `-z execstack/noexecstack` to emitted segment flag policy, and emits runpath using new-dtags mode (`DT_RUNPATH` vs `DT_RPATH`).
- shared-object ingestion for COFF and Mach-O is no longer treated as a no-op in host runtime linker paths.
- script statement handling now uses block-aware splitting instead of naive `;`/newline splitting, reducing false splits inside structured linker-script blocks.
- compatibility flags previously consumed through generic no-op paths now route into explicit linker state updates (`hash-style`, thread count, eh-frame header, diagnostics, print-gc, and icf mode).
- staged ELF write path now performs complete image emission in the header/finalize sequence, and per-section emit callbacks validate section stream bounds.
- host CLI no longer rejects several previously unsupported-but-common lld compatibility spellings in the script/export and diagnostics families.
- no-value `--print-map` now auto-derives a map output path (`a.out.map` for default output).
- linker-link profile naming now uses `LINKER_INTERNAL` while preserving `LINKER_INTERNAL_MVP` as a compatibility alias in Dust modules.
- split-form `--dynamic-linker <path>` and `--soname <name>` parsing now routes through requires-value handling in host CLI logic.

## 2026-02-20

### Added

- Object-format probing and ingest routing for:
  - ELF64 relocatable objects
  - COFF x86_64 objects
  - Mach-O 64-bit x86_64 objects
- Archive member kind detection for mixed archives (`.a` / `.lib` workflows).
- Minimal PE and Mach-O executable output writer paths.
- Basic linker script application support for:
  - `ENTRY(...)`
  - `OUTPUT_FORMAT(...)`
  - `SEARCH_DIR(...)`
  - `INPUT(...)`
  - `GROUP(...)`

### Changed

- Expanded `--oformat` surface to include `pe` and `macho64` aliases.
- Expanded host CLI compatibility options:
  - additional alias forms (`--entry-point`, `--library-path`, `--library`, `--map-file`)
  - accepted compatibility flags consumed as no-op/value-skip where not yet semantically implemented.
- Added script-aware final output resolution so script `OUTPUT_FORMAT`/`ENTRY` can influence final link configuration when CLI did not explicitly override those values.
- Updated documentation to reflect current capabilities and known limits.

### Notes

- `dustlink` remains an internal Dust linker, not a backend wrapper.
- Full `lld` drop-in parity is still in progress.
