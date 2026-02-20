# linker_image

Source: `src/linker_image.ds`

## Forge

`forge LinkerImage`

## Format Constants

- `FORMAT_FLAT = 1`
- `FORMAT_ELF = 2`
- `FORMAT_MBR = 3`

## Image Constants

- `IMAGE_MAX_SIZE = 33554432`
- `DEFAULT_TEXT_ADDR = 65536`
- `DEFAULT_DATA_ADDR = 1073741824`
- `DEFAULT_ENTRY_ADDR = 1048576`
- `DEFAULT_IMAGE_BUFFER = 4194304`

## `K` Domain Procedures

- `create_image(format) -> UInt64`
- `set_image_format(format) -> UInt32`
- `set_image_entry(entry) -> UInt32`
- `set_image_base(base) -> UInt32`
- `add_section(section) -> UInt32`
- `calculate_image_size() -> UInt32`
- `allocate_image(size) -> UInt64`
- `write_image(output) -> UInt32`
- `finalize_image() -> UInt32`
- `set_image_arch(arch) -> UInt32`
- `set_image_os(os) -> UInt32`
- `create_boot_image(kernel, kernel_size, output) -> UInt32`
- `create_efi_image(output) -> UInt32`
- `format_supported(format) -> UInt32`

`K` domain now validates format, base/entry constraints, image sizing, and basic output constraints.
`create_efi_image` currently returns `ERR_NOT_IMPLEMENTED_YET` after output validation.

## `Q` and `Phi`

- write/config/build procedures generally return `100`.
- `create_image`, `calculate_image_size`, `allocate_image` return `0` placeholders.
