# CLI Reference

Source: `src/main.ds`

## Binary

`dustlink`

## Internal Mode

`dustlink` is internal-linker mode only. It does not forward arguments to external linker backends.

## Exit Semantics

- Returns DustLink status code (`0` success, non-zero linker error code).
- Status code model is defined in `src/linker_errors.ds`.

## Internal Dust MVP CLI Profile

Sources: `src/linker_cli.ds`, `src/linker_host_cli.ds`

The internal Dust linker profile recognizes canonical/alias families for:

- output (`-o`, `--output`)
- entry (`-e`, `--entry`, `--entry-point`)
- map (`-Map`, `--Map`, `--map-file`)
- machine (`-m` class via canonical IDs)
- image base (`--image-base`)
- text address (`-Ttext`)
- script (`-T`, `--script`)
- oformat (`--oformat`)
- library path/library (`-L`, `--library-path`, `-l`, `--library`)
- strip (`-s`, `--strip-debug`)
- section GC (`--gc-sections`)
- multiple definition policy (`--allow-multiple-definition`)
- group flags (`--start-group`, `--end-group`)
- utility flags (`--help`, `--version`)

### OFormat values

`--oformat` supports:

- `elf64`, `elf64-x86-64`
- `binary`, `bin`
- `mbr`
- `pe`, `pei-x86-64`, `pe-x86-64`
- `macho64`, `mach-o`, `macho-x86-64`

### Compatibility handling

Several lld-style flags are accepted for compatibility and currently consumed as no-op/value-skip behaviors (for example `--build-id`, `--threads=*`, `--target=*`, `--sysroot=*`).

`linker_cli.ds` and `linker_host_cli.ds` return typed linker errors for unsupported flags and missing values.
