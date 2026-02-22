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

## Current Limitations

- Tests still focus on deterministic status behavior and arithmetic/helper semantics.
- Compatibility no-op diagnostics and fatal-warning escalation behavior are only partially covered by unit-style Dust tests and need broader CLI integration assertions.
- They do not yet validate exhaustive byte-for-byte parity against `lld` across full flag/script/cross-format matrices.
- Cross-platform host-link attempt ordering is not yet validated by integration tests.

## Suggested Next Additions

- Add integration tests for compiler host-link attempt ordering (`dustlink` preference and `lld`/`rust-lld` bootstrap path for `dustlink.exe`).
- Add fixture suites for Mach-O input objects and mixed archive member sets.
- Add script parser compliance tests beyond the current directive subset.
- Add byte-level fixture checks for PE/Mach-O headers and relocation patch results.
