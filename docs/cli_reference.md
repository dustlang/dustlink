# CLI Reference

Source: `src/main.rs`

## Binary

`dustlink`

## Help and Version

```text
dustlink -h
dustlink --help
dustlink -V
dustlink --version
```

Behavior:

- If no args are provided, help is printed.
- Version string is currently `0.1.0`.

## Special Flag

```text
dustlink --print-backend
```

- Resolves backend using normal discovery order.
- Prints absolute backend path.
- Exits without invoking linker.

## Pass-Through Mode

All other arguments are forwarded to the resolved linker backend.

Example:

```text
dustlink -o kernel.bin --entry 0x100000 a.o b.o
```

## Exit Semantics

- On successful backend process launch, `dustlink` exits with backend's exit code.
- On launch/resolution failures, `dustlink` prints `dustlink: <error>` to stderr and exits with code `1`.

## rust-lld Flavor Injection

When resolved backend executable name is `rust-lld` or `rust-lld.exe`:

- If no `-flavor`/`--flavor` flag is present, `dustlink` prepends:

```text
-flavor gnu
```

- If any flavor flag is already present, no injection occurs.

## Internal Dust MVP CLI Profile

Source: `src/linker_cli.ds`

The internal Dust linker profile currently recognizes canonical/alias families for:

- output (`-o`, `--output`)
- entry (`-e`, `--entry`)
- map (`-Map`)
- machine (`-m` class via canonical IDs)
- image base (`--image-base`)
- text address (`-Ttext`)
- oformat (`--oformat`)
- library path/library (`-L`, `-l`)
- strip (`-s`, `--strip-debug`)
- section GC (`--gc-sections`)
- multiple definition policy (`--allow-multiple-definition`)
- group flags (`--start-group`, `--end-group`)
- utility flags (`--help`, `--version`)

`linker_cli.ds` normalizes aliases to canonical flag IDs and returns typed linker errors for unsupported flags and missing values.
