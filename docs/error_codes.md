# Error Codes

Source: `src/linker_errors.ds`

## LinkerErrors Constants

- `ERR_OK = 0`
- `ERR_FILE_NOT_FOUND = 1`
- `ERR_INVALID_FORMAT = 2`
- `ERR_INVALID_SECTION = 3`
- `ERR_UNDEFINED_SYMBOL = 4`
- `ERR_MULTIPLE_DEFINITION = 5`
- `ERR_INVALID_RELOCATION = 6`
- `ERR_OUT_OF_MEMORY = 7`
- `ERR_INVALID_ADDRESS = 8`
- `ERR_INVALID_ENTRY = 9`
- `ERR_WRITE_FAILED = 10`
- `ERR_INVALID_IMAGE = 11`

## Domain Availability Convention

Across Dust modules, unsupported domains (`Q`, `Phi`) generally return `100` for mutating/operational procedures.

`100` is not currently defined in `LinkerErrors` and is used as a shared domain-unavailable sentinel in module code.
