# linker_link

Source: `src/linker_link.ds`

## Forge

`forge LinkerLink`

## Linker Constants

- `LINKER_NONE = 0`
- `LINKER_INTERNAL = 1`
- `LINKER_INTERNAL_MVP = 1`
- `LINKER_EXTERNAL_COMPAT = 2`

## Output Format Constants

- `FORMAT_ELF64 = 1`
- `FORMAT_FLAT = 2`
- `FORMAT_MBR = 3`
- `FORMAT_PE64 = 4`
- `FORMAT_MACHO64 = 5`

## Object Format Routing Constants

- `OBJECT_FORMAT_UNKNOWN = 0`
- `OBJECT_FORMAT_ELF64 = 1`
- `OBJECT_FORMAT_COFF64 = 2`
- `OBJECT_FORMAT_MACHO64 = 3`

## Target Constants

- `TARGET_X86_64_NONE = 0`
- `TARGET_X86_64_LINUX = 1`
- `TARGET_X86_64_WINDOWS = 2`
- `TARGET_X86_64_MACOS = 3`

## `K` Domain Procedures

- `add_object_file(path) -> UInt32`
- `add_library(path) -> UInt32`
- `add_search_path(path) -> UInt32`
- `set_linker(linker) -> UInt32`
- `set_target(target) -> UInt32`
- `set_output(output) -> UInt32`
- `set_entry(entry) -> UInt32`
- `set_base_address(base) -> UInt32`
- `link() -> UInt32`
- `link_with_script(script) -> UInt32`
- `ingest_object_by_format(path, object_kind) -> UInt32`
- `create_flat_binary(output, entry) -> UInt32`
- `create_mbr_image(output, boot_sector, kernel) -> UInt32`
- `get_linker_path() -> UInt64`
- `find_linker() -> UInt32`
- `is_supported_target(target) -> UInt32`
- `is_supported_format(format) -> UInt32`

`K` domain now performs deterministic input/target/format validation, probes object format via host runtime, routes ingest across ELF/COFF/Mach-O handlers, and returns `LinkerErrors` status codes.
`link_with_script(script)` applies script directives through the host runtime script parser path.
`add_library(name)` routes through search-path resolution that now supports:

- dynamic-mode shared-object preference for `-l` resolution,
- static-mode archive-only resolution,
- exact-name `-l:<file>` tokens,
- deterministic search order across explicit `-L`, dynamic `-rpath-link`, and target/sysroot defaults.

`find_linker()` resolves to `LINKER_INTERNAL` (`LINKER_INTERNAL_MVP` kept as compatibility alias).

## `Q` and `Phi`

- operational methods return `100`.
- `get_linker_path()` returns `0` placeholder.
