# DustLink Forge API

Source: `src/dustlink.ds`

## Forge

`forge DustLink`

## Version Constants

- `VERSION_MAJOR: UInt8 = 0`
- `VERSION_MINOR: UInt8 = 1`

## `K` Domain Procedures

- `link(input_files, output_file, format) -> UInt32`
- `link_with_options(input_files, output_file, format, entry, base) -> UInt32`
- `add_input_file(path) -> UInt32`
- `set_output(path) -> UInt32`
- `set_output_format(format) -> UInt32`
- `set_entry_point(addr) -> UInt32`
- `set_base_address(addr) -> UInt32`
- `add_search_path(path) -> UInt32`
- `add_library(name) -> UInt32`
- `run() -> UInt32`
- `get_error() -> UInt64`
- `print_sections() -> UInt32`
- `print_symbols() -> UInt32`
- `print_relocations() -> UInt32`
- `strip_debug() -> UInt32`
- `create_map_file(path) -> UInt32`
- `verify_image() -> UInt32`

Current return behavior: all `K` procedures in this file return `0` placeholders.

## `Q` and `Phi` Domain Procedures

Same API surface as `K`, with current behavior:

- operational procedures return `100` (`domain not available` convention)
- `get_error()` returns `0`

## Notes

The forge defines a stable linker API shape for Dust callers, but does not yet implement full linker state transitions in this module.
