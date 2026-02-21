# dustlink Changelog

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
