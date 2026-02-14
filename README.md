# DustLink

DPL Linker for creating bootable x86-64 binaries.

## Overview

DustLink is a linker written in the Dust Programming Language (DPL) that produces bootable binaries for x86-64 systems. It supports multiple output formats including:

- **Flat binary** - For boot sectors (MBR)
- **ELF64** - Executable and Linkable Format
- **MBR boot image** - 512-byte bootable sectors

## Features

- ELF64 format support
- Flat binary format for boot sectors
- Relocation processing (x86-64 relocations)
- Section management (.text, .data, .rodata, .bss)
- Symbol table management
- Boot image creation for VirtualBox/QEMU
- Multiple output formats

## Architecture

This implementation follows the K-Domain only approach (classical x86-64 hardware), with Q/Φ domains stubbed to return ERR_DOMAIN_NOT_AVAILABLE (100).

## Source Files

```
src/
├── linker_errors.ds      # Error codes
├── linker_elf.ds        # ELF binary format
├── linker_flat.ds       # Flat binary format
├── linker_reloc.ds      # Relocation processing
├── linker_sections.ds   # Section management
├── linker_symbol.ds     # Symbol table
├── linker_image.ds      # Final image creation
└── dustlink.ds          # Main linker module
```

## Supported Formats

### ELF64
- x86-64 machine type
- Executable and relocatable types
- Section headers and program headers
- Symbol and relocation tables

### Flat Binary
- Raw memory image
- MBR boot sector (512 bytes)
- Kernel flat image
- Configurable load addresses

### Boot Image
- 512-byte MBR with 0x55AA signature
- Bootloader + kernel in one image
- Suitable for VirtualBox/QEMU

## Memory Layout

```
0x00000 - 0xFFFFF    : Real mode (BIOS area)
0x07C00 - 0x07DFF    : Boot sector load address
0x07E00 - 0x9FFFF    : Boot stack/data
0x100000              : Kernel load address (1MB)
```

## Usage

DustLink takes object files from the DPL compiler and produces a linked binary:

```dust
// Example linker invocation
DustLink::K::link(input_files, output_file, FORMAT_ELF);
DustLink::K::set_entry_point(1048576);
DustLink::K::run();
```

## Domain Support

| Domain | Status |
|--------|--------|
| K      | Full implementation |
| Q      | Stubbed (returns ERR_DOMAIN_NOT_AVAILABLE) |
| Φ      | Stubbed (returns ERR_DOMAIN_NOT_AVAILABLE) |

## Error Codes

- 0: Success
- 1: ERR_FILE_NOT_FOUND
- 2: ERR_INVALID_FORMAT
- 3: ERR_INVALID_SECTION
- 4: ERR_UNDEFINED_SYMBOL
- 5: ERR_MULTIPLE_DEFINITION
- 6: ERR_INVALID_RELOCATION
- 7: ERR_OUT_OF_MEMORY
- 8: ERR_INVALID_ADDRESS
- 9: ERR_INVALID_ENTRY
- 10: ERR_WRITE_FAILED
- 100: ERR_DOMAIN_NOT_AVAILABLE

## Building

This project uses the DPL build system. Verify source files compile:

```bash
dust check src/
```

## Version

0.1.0
