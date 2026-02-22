# CLI Reference

Source: `src/main.ds`

## Binary

`dustlink`

## Internal Mode

`dustlink` is internal-linker mode only. It does not forward arguments to external linker backends.

## Exit Semantics

- Returns DustLink status code (`0` success, non-zero linker error code).
- Status code model is defined in `src/linker_errors.ds`.

## Internal Dust CLI Profile

Sources: `src/linker_cli.ds`, `src/linker_host_cli.ds`

The internal Dust linker profile recognizes canonical/alias families for:

- output (`-o`, `--output`)
- entry (`-e`, `--entry`, `--entry-point`)
- map (`-Map`, `--Map`, `--map-file`)
- machine (`-m` class via canonical IDs)
- image base (`--image-base`)
- text address (`-Ttext`)
- script (`-T`, `--script`)
- oformat (`--oformat`)
- library path/library (`-L`, `--library-path`, `-l`, `--library`)
- sysroot (`--sysroot`)
- runtime search path (`-rpath`, `--rpath`)
- link-time runtime search path (`-rpath-link`, `--rpath-link`)
- strip (`-s`, `--strip-debug`)
- section GC (`--gc-sections`)
- multiple definition policy (`--allow-multiple-definition`)
- group flags (`--start-group`, `--end-group`)
- utility flags (`--help`, `--version`)
- dynamic-policy flags (`--no-undefined`, `--error-unresolved-symbols`, `--allow-shlib-undefined`, `--no-allow-shlib-undefined`)
- dynamic-tag mode flags (`--enable-new-dtags`, `--disable-new-dtags`)
- transitive-needed flags (`--copy-dt-needed-entries`, `--no-copy-dt-needed-entries`)
- script/export compatibility flags (`--version-script`, `--dynamic-list`, `--trace-symbol`)
- broader compatibility/no-op controls (`--print-map`, `--start-lib`, `--end-lib`, `--emit-relocs`, `--strip-all`)
- `lld-link` forms (`/OUT:`, `/ENTRY:`, `/MACHINE:`, `/LIBPATH:`, `/DEFAULTLIB:`, `/MAP[:file]`, `/DLL`, `/SUBSYSTEM:`, `/OPT:`, `/WX`, `/NOENTRY`, `/DYNAMICBASE`, `/NXCOMPAT`, `/LARGEADDRESSAWARE`)

### OFormat values

`--oformat` supports:

- `elf64`, `elf64-x86-64`
- `binary`, `bin`
- `mbr`
- `pe`, `pei-x86-64`, `pe-x86-64`
- `macho64`, `mach-o`, `macho-x86-64`

### Compatibility handling

Compatibility spellings are accepted for common ld/lld flag families. Most linker-affecting compatibility flags are now state-wired instead of being generic no-op/value-skip paths.

State-wired (non-no-op) controls include:

- `--build-id[=<mode>]`
- `-z <token>`
- `--target` / `-m`
- `--defsym`
- `-u` / `--undefined` / `--require-defined`
- `--no-undefined` / `--error-unresolved-symbols` / `--allow-shlib-undefined` / `--no-allow-shlib-undefined`
- `-shared` / `-pie` / `--no-pie` / `-static` / `-Bstatic` / `-Bdynamic`
- `--dynamic-linker` / `--soname`
- `--sysroot`
- `-rpath` / `--rpath`
- `-rpath-link` / `--rpath-link`
- `--enable-new-dtags` / `--disable-new-dtags`
- `--copy-dt-needed-entries` / `--no-copy-dt-needed-entries`
- `--version-script` / `--dynamic-list`
- `--trace-symbol`
- `--print-map`
- `--start-lib` / `--end-lib`
- `--emit-relocs`
- `--strip-all`
- `--hash-style`
- `--threads` / `--thread-count`
- `--eh-frame-hdr`
- `--fatal-warnings` / `--no-fatal-warnings`
- `--color-diagnostics` / `--no-color-diagnostics`
- `--print-gc-sections` / `--no-print-gc-sections`
- `--icf=none` / `--icf=safe` / `--icf=all`
- `/DLL`
- `/MACHINE:<arch>`
- `/OPT:REF` / `/OPT:NOREF` / `/OPT:ICF` / `/OPT:NOICF`
- `/WX` / `/WX:NO`
- `/NOENTRY` / `/DYNAMICBASE` / `/NXCOMPAT` / `/LARGEADDRESSAWARE`

Accepted compatibility/no-op controls now also include common `lld-link` metadata families (`/PDB:`, `/IMPLIB:`, `/MANIFEST:`, `/EXPORT:`, `/NODEFAULTLIB[:...]`, `/INCLUDE:`) and long-form inline options such as `--plugin-opt=*`, `--mllvm=*`, and `--thinlto-*`.

`linker_cli.ds` and `linker_host_cli.ds` return typed linker errors for unsupported flags and missing values.
