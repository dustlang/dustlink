# Testing

## Source Files

- `src/dustlink_tests.ds`
- `src/linker_link_tests.ds`
- `src/linker_host_cli_tests.ds`
- `src/linker_script_semantics_tests.ds`
- `src/linker_buildid_z_tests.ds`

## Test Structure

Both files define forge-based test procedures with this pattern:

- call target procedure with deterministic inputs
- compare against expected linker error/status constants
- return `1` for pass
- `run_all_tests()` aggregates results by summing each case result

## Coverage Themes

- `K` domain validation behaviors (input/output/format/entry/base/target checks)
- CLI compatibility helpers (flag normalization and missing-value error paths)
- `Q` and `Phi` domain error-path expectations (`100`)
- Linker-link integration surface coverage (`set_linker`, `set_target`, `create_flat_binary`, `find_linker`, etc.)
- Cross-format compatibility checks (ELF/PE/Mach-O oformat and support routing)
- Script semantic checks (`OUTPUT_FORMAT`, `PROVIDE`, `EXTERN`, `INCLUDE`)
- Script subset checks for `MEMORY` (`ORIGIN`) and `SECTIONS` location-counter assignment
- Script subset checks for `SECTIONS` output-address forms and multi-line block parsing
- Script directive checks for `OUTPUT_ARCH` and `AS_NEEDED` state scoping
- Script directive checks for `OUTPUT_ARCH` AArch64 target mapping
- Script `ENTRY(symbol)` required-symbol registration checks
- Script checks for sysroot-aware `SEARCH_DIR(=...)` resolution
- Script checks for `INPUT(-L...)` search-path updates and `INPUT(-l...)` needed-library recording
- Script expression checks (`ORIGIN/LENGTH` arithmetic) and `ASSERT(...)` failure path checks
- Script expression checks for shift/bitwise operator evaluation and direct symbol-assignment statements
- Script compatibility-block acceptance checks for `PHDRS` and `VERSION`
- Script strictness checks for unknown-directive rejection
- Build-id mode and `-z` option semantic state checks
- PE compatibility-state checks for `/NOENTRY`, `/DYNAMICBASE`, `/NXCOMPAT`, `/LARGEADDRESSAWARE`
- Dynamic unresolved-policy checks (`--no-undefined` and allow-shared-unresolved gate behavior)
- Search-path state checks (`--sysroot`, `-rpath`, `-rpath-link`)
- Dynamic tag mode checks (`--enable-new-dtags`, `--disable-new-dtags`)
- Copy-needed policy checks (`--copy-dt-needed-entries`, `--no-copy-dt-needed-entries`)
- AArch64 relocation helper/validation checks for:
  - branch/literal/ADR/ADRP/ADD/LDST bitfield patch helpers
  - `LDST128` scaling/alignment behavior
  - `ADR_PREL_LO21` and MOVW patch/validation paths
  - starter AArch64 TLS data relocation value validation (`TLS_DTPMOD`, `TLS_TPREL`)
  - `TLSDESC_CALL` instruction-shape validation (`BLR` accepted, `BR` rejected)
  - TLSLE/TLSLD low12 offset relocation-kind mapping helper coverage

## Recent Integration Checks

- Dust host executable build succeeds from `src/main.ds`.
- PE and Mach-O writer paths emit valid file magics (`MZ`, `CFFAEDFE`).
- PE and Mach-O writer paths now emit sectionized images from alloc chunks rather than single synthetic text payloads.
- COFF object ingestion path links into PE output.
- Script path (`-T/--script`) applies basic directives and can override output format.
- `-Map` output creation works on split form (`-Map file.map`).
- Required-symbol CLI paths (`-u`, `--undefined`, `--require-defined`) are parsed and wired to enforcement checks.
- Dynamic unresolved-policy CLI paths (`--no-undefined`, `--allow-shlib-undefined`, `--no-allow-shlib-undefined`) are parsed and wired to linker state.
- `-z` semantic tests include unresolved-policy toggles (`defs`/`undefs`) and accepted compatibility tokens (`text`/`notext`/`origin`).
- Search-path CLI paths (`--sysroot`, `-rpath`, `-rpath-link`) are parsed and wired to linker state.
- Dynamic tag and copy-needed policy flags are parsed and wired to linker state.
- Shared-object symbol ingestion now includes ELF, PE, COFF, and Mach-O metadata paths.
- Relocation handling coverage now includes additional x86_64 relocation IDs used in broader compatibility workflows.
- CLI compatibility state controls (`--hash-style`, `--threads`, `--thread-count`) are parsed in inline and split flag forms.
- Extended CLI compatibility controls (`--version-script`, `--dynamic-list`, `--trace-symbol`, `--dependency-file`, `--print-map`, `--start-lib`, `--end-lib`) are parsed in inline/split or no-value forms as applicable.
- `lld-link` compatibility parser helpers (`/OUT`, `/ENTRY`, `/MACHINE`, `/LIBPATH`, `/DEFAULTLIB`, `/MAP`, `/DLL`, and compatibility inline/no-value families) have explicit host-CLI unit coverage.
- Linker script tests now include malformed `PHDRS`/`VERSION` block rejection coverage.
- Host CLI tests now include ARM64 `/MACHINE` mapping and soft-compatibility flag-family detection (`--time-trace-file=*`, `/GUARD:*`).
- Host CLI tests now include musl/Windows GNU/bare-metal target alias coverage.
- Linker core tests include AArch64 target acceptance in internal target validation.
- `dust check src` passes for current Dust-source linker modules (host CLI + runtime parity changes).
- `dust check src` passes after AArch64 relocation parity expansions (MOVW, `ADR_PREL_LO21`, and starter TLS relocation plumbing).
- `dust check src` passes after AArch64 TLS data-reloc host-helper wiring (non-shared links), stricter TLS instruction-family `ERR_NOT_IMPLEMENTED_YET` handling, and shared-ingest error propagation/format strictness tightening.
- `dust check src` passes after adding AArch64 TLSLE/TLSLD low12 offset relocation routing via host TLS helpers (non-shared links).
- `dust check src` passes after enabling `R_AARCH64_TLSDESC_CALL` application (validated preserve relocation) within the remaining partial AArch64 TLS-family support.
- `cargo check -p dust_codegen` passes after host shared-library metadata parsing additions for embedded needed-name preference (`SONAME` / export DLL name / Mach-O install name).
- `cargo check -p dust_codegen` passes after host AArch64 shared-link TLS data-reloc behavior differentiation (`TLS_DTPREL` support, shared-link `TLS_TPREL` invalid) and shared-export filtering tightening for ELF/Mach-O metadata paths.
- `cargo check -p dust_codegen` and `dust check src` pass after adding preparatory AArch64 TLS synthetic descriptor/GOT planning state + host helper ABI and Dust-side TLS planning-slot reservation calls for unsupported descriptor-sequence relocs.
- `cargo check -p dust_codegen` and `dust check src` pass after wiring AArch64 TLS descriptor-sequence instruction reloc application through the host synthetic-slot reloc-value helper, adding synthetic TLS slot-region materialization in ELF load images, and emitting minimal synthetic `.rela.dyn` metadata (`DT_SYMTAB`, `DT_SYMENT`, `DT_RELA*`) for reserved descriptor-sequence slots.
- `cargo check -p dust_codegen` and `dust check src` pass after:
  - symbol-aware `--as-needed` shared-object retain/drop handling with rollback of dropped-library symbol state
  - host query wiring for retained shared-object state (`host_linker_last_shared_object_retained`)
  - deterministic versioned shared-library fallback lookup (`lib<name>.so.<N>` / `lib<name>.dylib.<N>`) in directory search paths.

## Current Limitations

- Tests still focus on deterministic status behavior and arithmetic/helper semantics.
- Compatibility no-op diagnostics and fatal-warning escalation behavior are only partially covered by unit-style Dust tests and need broader CLI integration assertions.
- AArch64 TLS descriptor-sequence staged semantics (synthetic slot-region emission, synthetic `.rela.dyn` metadata, and relocation application wiring) are not yet covered by byte-level ELF fixture tests or runtime TLS behavior tests.
- They do not yet validate exhaustive byte-for-byte parity against `lld` across full flag/script/cross-format matrices.
- Cross-platform host-link attempt ordering is not yet validated by integration tests.

## Suggested Next Additions

- Add integration tests for compiler host-link attempt ordering (`dustlink` preference and `lld`/`rust-lld` bootstrap path for `dustlink.exe`).
- Add fixture suites for Mach-O input objects and mixed archive member sets.
- Add script parser compliance tests beyond the current directive subset.
- Add byte-level fixture checks for PE/Mach-O headers and relocation patch results.
