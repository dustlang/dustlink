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
- Script `ENTRY(symbol)` required-symbol registration checks
- Build-id mode and `-z` option semantic state checks
- Dynamic unresolved-policy checks (`--no-undefined` and allow-shared-unresolved gate behavior)
- Search-path state checks (`--sysroot`, `-rpath`, `-rpath-link`)
- Dynamic tag mode checks (`--enable-new-dtags`, `--disable-new-dtags`)
- Copy-needed policy checks (`--copy-dt-needed-entries`, `--no-copy-dt-needed-entries`)

## Recent Integration Checks

- Dust host executable build succeeds from `src/main.ds`.
- PE and Mach-O writer paths emit valid file magics (`MZ`, `CFFAEDFE`).
- COFF object ingestion path links into PE output.
- Script path (`-T/--script`) applies basic directives and can override output format.
- `-Map` output creation works on split form (`-Map file.map`).
- Required-symbol CLI paths (`-u`, `--undefined`, `--require-defined`) are parsed and wired to enforcement checks.
- Dynamic unresolved-policy CLI paths (`--no-undefined`, `--allow-shlib-undefined`) are parsed and wired to linker state.
- Search-path CLI paths (`--sysroot`, `-rpath`, `-rpath-link`) are parsed and wired to linker state.
- Dynamic tag and copy-needed policy flags are parsed and wired to linker state.
- Shared-object symbol ingestion now includes ELF, PE, COFF, and Mach-O metadata paths.
- Relocation handling coverage now includes additional x86_64 relocation IDs used in broader compatibility workflows.

## Current Limitations

- Tests still focus on deterministic status behavior and MVP arithmetic helpers.
- They do not yet validate exhaustive byte-for-byte parity against `lld` across full flag/script/cross-format matrices.
- Cross-platform host-link attempt ordering is not yet validated by integration tests.

## Suggested Next Additions

- Add integration tests for compiler host-link attempt ordering (`dustlink` preference and `lld`/`rust-lld` bootstrap path for `dustlink.exe`).
- Add fixture suites for Mach-O input objects and mixed archive member sets.
- Add script parser compliance tests beyond the current directive subset.
- Add byte-level fixture checks for PE/Mach-O headers and relocation patch results.
