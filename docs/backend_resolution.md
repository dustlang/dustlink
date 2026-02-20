# Backend Resolution

Source: `src/main.rs`

## Resolution Order

`dustlink` resolves backend in this exact order:

1. `DUSTLINK_BACKEND` environment variable.
2. `ld.lld` in `PATH`.
3. `rust-lld` in `PATH`.
4. `rust-lld` from Rust sysroot discovered through `rustc`.

If all fail, `dustlink` returns:

```text
no linker backend found; install ld.lld or rustup toolchain with rust-lld
```

## `DUSTLINK_BACKEND` Rules

If env value contains a path separator or is absolute:

- Must point to an existing file.
- Otherwise fails with missing-file error.

If env value is a bare command name:

- Searched in `PATH`.
- Fails if command is not found.

## PATH Search Behavior

- Non-Windows: checks `<dir>/<command>` across `PATH` entries.
- Windows: also tries PATHEXT extensions (`EXE`, `CMD`, `BAT` fallback when `PATHEXT` unavailable).

## Sysroot Fallback

`dustlink` executes:

- `rustc --print sysroot`
- `rustc -vV` (to parse `host:` triple)

Then probes typical rust-lld locations in sysroot, including host-specific and generic rustlib bin directories.
