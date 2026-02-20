# Testing

## Source Files

- `src/dustlink_tests.ds`
- `src/linker_link_tests.ds`

## Test Structure

Both files define forge-based test procedures with this pattern:

- call target procedure
- return `1` for pass, or conditionally return `1` when expected domain error (`100`) is observed
- `run_all_tests()` aggregates results by summing each case result

## Coverage Themes

- `K` domain API presence calls
- `Q` and `Phi` domain error-path expectations (`100`)
- Linker-link integration surface coverage (`add_object_file`, `set_target`, `link`, etc.)

## Current Limitations

- Most tests verify return-code conventions only.
- They do not validate real object parsing, relocation patching, section layout, or emitted binary bytes.
- Rust CLI behavior in `src/main.rs` is not covered by Rust integration tests in this package.

## Suggested Next Additions

- Add Rust integration tests for backend resolution and `--print-backend`.
- Add Dust tests for non-placeholder `K` implementations as linker logic is filled in.
- Add fixture-based link outputs for ELF/flat/MBR byte-level verification.
