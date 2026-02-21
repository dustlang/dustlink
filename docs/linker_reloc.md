# linker_reloc

Source: `src/linker_reloc.ds`

## Forge

`forge LinkerReloc`

## Constants

- `MAX_RELOCATIONS = 131072`
- `MAX_SECTION_INDEX = 4096`
- `R_X86_64_NONE = 0`
- `R_X86_64_64 = 1`
- `R_X86_64_PC32 = 2`
- `R_X86_64_PLT32 = 4`
- `R_X86_64_GLOB_DAT = 6`
- `R_X86_64_JUMP_SLOT = 7`
- `R_X86_64_RELATIVE = 8`
- `R_X86_64_GOTPCREL = 9`
- `R_X86_64_32 = 10`
- `R_X86_64_32S = 11`
- `R_X86_64_GOTPCRELX = 41`
- `R_X86_64_REX_GOTPCRELX = 42`

## `K` Domain Procedures

- `is_supported_reloc(reloc_type) -> UInt32`
- `reloc_write_width(reloc_type) -> UInt32`
- `validate_relocation_value(reloc_type, value) -> UInt32`
- `validate_relocation_record(section, offset, sym_idx, reloc_type, section_size, symbol_count) -> UInt32`
- `calculate_pcrel_offset(reloc, sym_addr, offset) -> UInt64`
- `calc_relocation_value(reloc_type, symbol_addr, addend, place_addr) -> UInt64`
- `apply_relocation_value(section_index, offset, reloc_type, value) -> UInt32`
- `apply_relocation_slot(object_index, reloc_index) -> UInt32`
- `apply_object_relocations(object_index, reloc_index, reloc_count) -> UInt32`
- `apply_all_relocations(object_index, object_count) -> UInt32`
- `process_relocations(section, relocs, symbols, data) -> UInt32`
- `resolve_reloc_address(reloc, sym_addr, base_addr) -> UInt64`

`K` domain validates relocation records, computes relocation values, and applies patch operations through host runtime patch intrinsics (`host_linker_patch_u32`/`host_linker_patch_u64`).

## `Q` and `Phi`

No `Q`/`Phi` operational relocation path is provided in this module.
