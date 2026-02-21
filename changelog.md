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

### Changed

- `--gc-sections` is no longer a no-op; output segment construction now applies GC-aware section retention.
- `--allow-multiple-definition` now affects global symbol selection conflict behavior.
- Link image planning now uses dynamic calculated image size instead of fixed placeholder section-size constants.
- Script `SEARCH_DIR` and `INPUT`/`GROUP` path tokens now resolve relative to the including script directory.
- Symbol resolution checks now enforce required symbol declarations after global resolution.
- ELF writer now includes build-id note payload when configured, and applies `-z execstack/noexecstack` to emitted segment flag policy.

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
