# linker_elf

Source: `src/linker_elf.ds`

## Forge

`forge ElfFormat`

## Selected Constants

- ELF magic bytes: `EI_MAG0..EI_MAG3`
- Class/data/version defaults: `EI_CLASS`, `EI_DATA`, `EI_VERSION`
- Machine: `EM_X86_64 = 62`
- File types: `ET_REL`, `ET_EXEC`, `ET_DYN`
- Section constants: `SHN_UNDEF`, `SHN_ABS`, `SHT_*`, `SHF_*`
- Symbol bind/type masks and values: `ELF64_ST_BIND`, `ELF64_ST_TYPE`, `STB_*`, `STT_*`
- Relocation types: `R_X86_64_NONE`, `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_32`, `R_X86_64_32S`

## `K` Domain Procedures

- `validate_elf_header(data) -> UInt32`
- `get_elf_class(data) -> UInt8`
- `get_elf_machine(data) -> UInt16`
- `get_elf_type(data) -> UInt16`
- `get_entry_point(data) -> UInt64`
- `get_section_count(data) -> UInt16`
- `get_program_header_count(data) -> UInt16`
- `get_section_header_offset(data) -> UInt64`
- `get_program_header_offset(data) -> UInt64`

Current return behavior in this file is placeholder-oriented (for example class/machine/type helpers return fixed values, most others return `0`).

## `Q` and `Phi`

Only partial helpers are present (`validate_elf_header`, `get_elf_class`), both returning placeholders.
