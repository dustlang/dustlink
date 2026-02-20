# Testing

## Source Files

- `src/dustlink_tests.ds`
- `src/linker_link_tests.ds`

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

## Current Limitations

- Tests still focus on deterministic status behavior and MVP arithmetic helpers.
- They do not yet validate full object parsing, relocation patching on byte buffers, section packing, or emitted file bytes.
- Rust CLI behavior in `src/main.rs` is not covered by Rust integration tests in this package.

## Suggested Next Additions

- Add Rust integration tests for backend resolution and `--print-backend`.
- Add Dust tests for non-placeholder `K` implementations as linker logic is filled in.
- Add fixture-based link outputs for ELF/flat/MBR byte-level verification.
