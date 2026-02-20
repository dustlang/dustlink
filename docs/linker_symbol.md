# linker_symbol

Source: `src/linker_symbol.ds`

## Forge

`forge LinkerSymbol`

## Capacity Constants

- `MAX_SYMBOLS = 8192`
- `MAX_SYMBOL_NAME = 128`

## Symbol Constants

- Visibility aliases: `SYM_LOCAL`, `SYM_GLOBAL`, `SYM_WEAK`
- Type constants: `TYPE_NONE`, `TYPE_OBJECT`, `TYPE_FUNC`, `TYPE_SECTION`, `TYPE_FILE`
- Binding constants: `BIND_LOCAL`, `BIND_GLOBAL`, `BIND_WEAK`

## `K` Domain Procedures

- `add_symbol(name, value, size, sym_type, bind, section) -> UInt32`
- `find_symbol(name) -> UInt32`
- `get_symbol(name) -> UInt64`
- `get_symbol_count() -> UInt32`
- `resolve_symbol(name) -> UInt64`
- `define_symbol(name, value) -> UInt32`
- `get_symbol_value(idx) -> UInt64`
- `get_symbol_section(idx) -> UInt32`
- `get_symbol_size(idx) -> UInt32`
- `is_defined(idx) -> UInt32`
- `resolve_undefined() -> UInt32`

Current behavior is placeholder returns (`0`).

## `Q` and `Phi`

- write/resolve procedures typically return `100`.
- read/query procedures return `0` placeholders.
