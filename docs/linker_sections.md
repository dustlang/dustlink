# linker_sections

Source: `src/linker_sections.ds`

## Forge

`forge LinkerSections`

## Section Constants

- `MAX_SECTIONS = 64`
- `MAX_SECTION_NAME = 32`
- `SECTION_TEXT = 0`
- `SECTION_DATA = 1`
- `SECTION_RODATA = 2`
- `SECTION_BSS = 3`
- `SECTION_DEBUG = 4`

## Flag Constants

- `FLAG_ALLOC = 1`
- `FLAG_WRITE = 2`
- `FLAG_EXEC = 4`

## `K` Domain Procedures

- `create_section(name, flags, align) -> UInt32`
- `get_section(name) -> UInt32`
- `get_section_count() -> UInt32`
- `add_section_data(section, data, size) -> UInt32`
- `set_section_address(section, addr) -> UInt32`
- `get_section_address(section) -> UInt64`
- `get_section_size(section) -> UInt32`
- `get_section_flags(section) -> UInt32`
- `get_section_alignment(section) -> UInt64`
- `merge_sections(a, b) -> UInt32`
- `calculate_section_offsets(base) -> UInt64`

Current behavior is placeholder returns (`0`).

## `Q` and `Phi`

- mutating operations generally return `100`.
- read/query operations generally return `0` placeholders.
