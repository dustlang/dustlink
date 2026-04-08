# linker_cli

Source: `src/linker_cli.ds`

## Forge

`forge LinkerCli`

## Purpose

Defines a linker-agnostic compatibility profile for common gcc/ld/lld-style arguments.

The module exposes:

- canonical flag IDs
- alias IDs for common spellings
- machine/oformat IDs
- normalization and validation helpers

## Canonical Flags

- `FLAG_OUTPUT`
- `FLAG_ENTRY`
- `FLAG_MAP`
- `FLAG_MACHINE`
- `FLAG_IMAGE_BASE`
- `FLAG_TEXT_ADDRESS`
- `FLAG_OFORMAT`
- `FLAG_NOSTDLIB`
- `FLAG_LIBRARY_PATH`
- `FLAG_LIBRARY`
- `FLAG_STRIP_DEBUG`
- `FLAG_GC_SECTIONS`
- `FLAG_ALLOW_MULTIPLE_DEFINITION`
- `FLAG_START_GROUP`
- `FLAG_END_GROUP`
- `FLAG_HELP`
- `FLAG_VERSION`

## Alias Examples

- `-o`, `--output`
- `-e`, `--entry`, `--entry-point`
- `-Map`, `--Map`, `--map-file`
- `--image-base`
- `-Ttext`
- `-T`, `--script`
- `--oformat`
- `-L`, `--library-path`
- `-l`, `--library`
- `-s`, `--strip-debug`
- `--gc-sections`
- `--allow-multiple-definition`
- `--no-undefined`, `--error-unresolved-symbols`, `--allow-shlib-undefined`
- `--sysroot`, `-rpath`, `--rpath`, `-rpath-link`, `--rpath-link`
- `--enable-new-dtags`, `--disable-new-dtags`
- `--copy-dt-needed-entries`, `--no-copy-dt-needed-entries`
- `--help`
- `--version`

## `K` Domain API

- `normalize_flag(flag) -> UInt32`
- `flag_requires_value(flag) -> UInt32`
- `is_supported_flag(flag) -> UInt32`
- `validate_argument(flag, has_value) -> UInt32`
- `normalize_machine(machine) -> UInt32`
- `machine_supported(machine) -> UInt32`
- `normalize_oformat(format) -> UInt32`
- `oformat_supported(format) -> UInt32`

Validation returns linker error codes from `LinkerErrors` for unsupported flags, missing flag values, and unsupported targets.

`oformat_supported` includes `ELF64`, `binary`, `MBR`, `PE64`, and `Mach-O64` IDs.

## `Q` and `Phi`

Domain-unavailable behavior remains (`100` on operational procedures).
