# linker_flat

Source: `src/linker_flat.ds`

## Forge

`forge FlatFormat`

## Constants

- `MBR_SIZE = 512`
- `MBR_SIGNATURE = 43660` (`0xAA55`)
- `BOOT_LOADER_ADDRESS = 32768`
- `KERNEL_LOAD_ADDRESS = 1048576`
- `MAX_BOOT_SIZE = 512`
- `MAX_KERNEL_SIZE = 16777216`

## `K` Domain Procedures

- `create_mbr_bootloader() -> UInt64`
- `validate_mbr(data) -> UInt32`
- `set_boot_signature(data) -> UInt32`
- `create_flat_image(sections, output, size) -> UInt32`
- `align_to(value, alignment) -> UInt64`
- `create_elf_image(sections, entry, output) -> UInt32`

`align_to` contains real arithmetic behavior:

- returns `value` when already aligned
- otherwise rounds up to next `alignment` boundary

Other `K` procedures return placeholder `0`.

## `Q` and `Phi`

- Most operational procedures return `100`.
- `create_mbr_bootloader()` returns `0` placeholder.
