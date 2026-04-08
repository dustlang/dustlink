# linker_symbol

Source: `src/linker_symbol.ds`

## Forge

`forge LinkerSymbol`

## Purpose

`LinkerSymbol` owns deterministic symbol resolution policy across loaded objects and host-side global symbol state.

## Constants (Current)

- `MAX_SYMBOLS = 131072`
- Binding aliases: `SYM_LOCAL`, `SYM_GLOBAL`, `SYM_WEAK`
- Type constants: `TYPE_NONE`, `TYPE_OBJECT`, `TYPE_FUNC`, `TYPE_SECTION`, `TYPE_FILE`
- Binding merge constants: `BIND_LOCAL`, `BIND_GLOBAL`, `BIND_WEAK`
- Section markers: `SHN_UNDEF`, `SHN_ABS`

## Key `K` Domain Procedures

- Table and record validation:
  - `validate_symbol_table_shape(symbol_count, name_bytes) -> UInt32`
  - `parse_symbol_bind(info) -> UInt8`
  - `parse_symbol_type(info) -> UInt8`
- Host-global interactions:
  - `find_symbol(name) -> UInt32`
  - `get_symbol(name) -> UInt64`
  - `define_symbol(name, value) -> UInt32`
- Resolution pipeline:
  - `resolve_loaded_symbols() -> UInt32`
  - `resolve_loaded_symbol_graph(object_index, object_count) -> UInt32`
  - `resolve_symbol_slot(object_index, symbol_index) -> UInt32`
  - `check_unresolved_globals() -> UInt32`

## Unresolved Policy Behavior

`check_unresolved_globals()` now has a dynamic-link-aware branch:

- If unresolved count is zero: enforce required-symbol checks.
- If unresolved count is non-zero:
  - when `host_linker_allow_dynamic_unresolved() == 1`, unresolved globals are permitted and required-symbol checks still run;
  - otherwise unresolveds are validated through `resolve_undefined(...)` with weak fallback handling.

This is how `--no-undefined` / `--allow-shlib-undefined` semantics are applied during final symbol validation.

## `Q` and `Phi`

Operational paths remain unavailable in `Q`/`Phi` domains and return domain error `100`.
