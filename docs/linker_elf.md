# linker_elf

Source: `src/linker_elf.ds`

## Forge

`forge ElfFormat`

## Selected Constants

- ELF magic bytes: `EI_MAG0..EI_MAG3`
- Class/data/version defaults: `EI_CLASS`, `EI_DATA`, `EI_VERSION`
- Machine: `EM_X86_64 = 62`, `EM_AARCH64 = 183`
- File types: `ET_REL`, `ET_EXEC`, `ET_DYN`
- Section constants: `SHN_UNDEF`, `SHN_ABS`, `SHT_*`, `SHF_*`
- Symbol bind/type masks and values: `ELF64_ST_BIND`, `ELF64_ST_TYPE`, `STB_*`, `STT_*`
- Relocation types:
  - `R_X86_64_NONE`, `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_PLT32`
  - `R_X86_64_GLOB_DAT`, `R_X86_64_JUMP_SLOT`, `R_X86_64_RELATIVE`
  - `R_X86_64_GOTPCREL`, `R_X86_64_32`, `R_X86_64_32S`
  - `R_X86_64_GOTPCRELX`, `R_X86_64_REX_GOTPCRELX`
  - selected AArch64 sets used by current Dust linker relocation ingest:
    - core/data (`ABS*`, `PREL*`)
    - branch/literal/ADR (`CALL26`, `JUMP26`, `CONDBR19`, `TSTBR14`, `LD_PREL_LO19`, `ADR_PREL_LO21`, `ADR_PREL_PG_HI21*`)
    - lo12 patch families (`ADD_ABS_LO12_NC`, `LDST*_ABS_LO12_NC`, `LDST128`)
    - MOVW (`UABS`, `SABS`, `PREL`)
    - starter TLS and TLSDESC instruction/data forms (`TLSGD`, `TLSLD`, `TLSDESC`, `TLS_DTPMOD`, `TLS_DTPREL`, `TLS_TPREL`)

## `K` Domain Procedures

- `validate_elf_identity(m0, m1, m2, m3, klass, endian, version) -> UInt32`
- `validate_elf_header(data) -> UInt32`
- `validate_machine(machine) -> UInt32`
- `validate_type(elf_type) -> UInt32`
- `validate_relocation_type(reloc_type) -> UInt32`
- `get_elf_class(data) -> UInt8`
- `get_elf_machine(data) -> UInt16`
- `get_elf_type(data) -> UInt16`
- `get_entry_point(data) -> UInt64`
- `get_section_count(data) -> UInt16`
- `get_program_header_count(data) -> UInt16`
- `get_section_header_offset(data) -> UInt64`
- `get_program_header_offset(data) -> UInt64`

The current implementation validates ELF identity, machine type, file type, and relocation type using System-V ELF64 constants. Machine validation now accepts `EM_X86_64` and `EM_AARCH64`. Relocation-type validation has been expanded to accept the current AArch64 relocation set used by `linker_reloc.ds`, including MOVW and starter TLS/TLSDESC forms. Getter helpers remain deterministic descriptor-based defaults.

## `Q` and `Phi`

Only partial helpers are present (`validate_elf_header`, `get_elf_class`), both returning placeholders.
