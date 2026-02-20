# linker_reloc

Source: `src/linker_reloc.ds`

## Forge

`forge LinkerReloc`

## Constants

- `MAX_RELOCATIONS = 16384`
- `R_X86_64_NONE = 0`
- `R_X86_64_64 = 1`
- `R_X86_64_PC32 = 2`
- `R_X86_64_32 = 10`
- `R_X86_64_32S = 11`

## `K` Domain Procedures

- `add_relocation(section, offset, sym_idx, reloc_type) -> UInt32`
- `apply_relocation(reloc, data, symbols) -> UInt32`
- `process_relocations(section, relocs, symbols, data) -> UInt32`
- `resolve_reloc_address(reloc, sym_addr, base_addr) -> UInt64`
- `calculate_pcrel_offset(reloc, sym_addr, offset) -> UInt64`

Current implementation returns placeholders in this file.

## `Q` and `Phi`

All listed procedures return `100`.
