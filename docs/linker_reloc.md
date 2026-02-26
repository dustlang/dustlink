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
- AArch64 relocation constants are also defined, including:
  - core/data: `R_AARCH64_NONE`, `R_AARCH64_ABS64`, `R_AARCH64_ABS32`, `R_AARCH64_PREL64`, `R_AARCH64_PREL32`
  - branch/literal/ADR forms: `R_AARCH64_CALL26`, `R_AARCH64_JUMP26`, `R_AARCH64_CONDBR19`, `R_AARCH64_TSTBR14`, `R_AARCH64_LD_PREL_LO19`, `R_AARCH64_ADR_PREL_LO21`, `R_AARCH64_ADR_PREL_PG_HI21`, `R_AARCH64_ADR_PREL_PG_HI21_NC`
  - lo12 forms: `R_AARCH64_ADD_ABS_LO12_NC`, `R_AARCH64_LDST8/16/32/64/128_ABS_LO12_NC`
  - MOVW families: `R_AARCH64_MOVW_UABS_*`, `R_AARCH64_MOVW_SABS_*`, `R_AARCH64_MOVW_PREL_*`
  - TLS families: `TLSGD`, `TLSLD`, `TLSDESC` instruction relocations plus `R_AARCH64_TLS_DTPMOD`, `R_AARCH64_TLS_DTPREL`, `R_AARCH64_TLS_TPREL`

## `K` Domain Procedures

- `is_supported_reloc(reloc_type, machine) -> UInt32`
- `reloc_write_width(reloc_type, machine) -> UInt32`
- `validate_relocation_value(reloc_type, machine, value) -> UInt32`
- `validate_relocation_record(section, offset, sym_idx, reloc_type, machine, section_size, symbol_count) -> UInt32`
- `calculate_pcrel_offset(reloc, sym_addr, offset) -> UInt64`
- `calc_relocation_value(reloc_type, machine, symbol_addr, addend, place_addr) -> UInt64`
- `apply_relocation_value(section_index, offset, reloc_type, machine, value) -> UInt32`
- `apply_relocation_slot(object_index, reloc_index) -> UInt32`
- `apply_object_relocations(object_index, reloc_index, reloc_count) -> UInt32`
- `apply_all_relocations(object_index, object_count) -> UInt32`
- `process_relocations(section, relocs, symbols, data) -> UInt32`
- `resolve_reloc_address(reloc, sym_addr, base_addr) -> UInt64`

`K` domain validates relocation records, computes relocation values, and applies patch operations through host runtime patch intrinsics (`host_linker_patch_u32`/`host_linker_patch_u64`).
Current AArch64 support includes instruction bitfield patching for branch/literal/ADR/ADRP/ADD/LDST relocations and MOVW family relocations.
AArch64 TLS-family relocations are recognized/validated in this module.
AArch64 TLS data relocs (`TLS_DTPMOD`, `TLS_DTPREL`, `TLS_TPREL`) are applied through a host-runtime helper that computes values from object TLS section metadata for non-shared links.
AArch64 shared-link TLS data relocation handling is partial but more specific: `TLS_DTPREL` can resolve from TLS layout metadata, shared-link `TLS_TPREL` returns invalid relocation, and shared-link `TLS_DTPMOD` remains not implemented.
AArch64 TLSLE/TLSLD low12 offset instruction relocations (`ADD`/`LDST64`/`LDST128` `*_DTPREL_*` / `*_TPREL_*`) also reuse the host-runtime TLS offset helper in non-shared links.
`R_AARCH64_TLSDESC_CALL` is applied as a validated `BLR` preserve relocation (no immediate payload rewrite).
Remaining AArch64 TLS instruction/descriptor-family relocations still return `ERR_NOT_IMPLEMENTED_YET` until full TLS descriptor/GOT metadata and relaxation semantics are exposed by the host linker runtime ABI.
`R_AARCH64_TLSDESC_CALL` helper now validates `BLR` instruction encoding before preserving the instruction word.

## `Q` and `Phi`

No `Q`/`Phi` operational relocation path is provided in this module.
