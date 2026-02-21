# Architecture

## Workspace Layout

- `src/main.ds`: Dust-native internal linker entrypoint.
- `src/dustlink.ds`: top-level Dust linker forge API.
- `src/linker_*.ds`: Dust module APIs for CLI normalization, object/archive intake, section/symbol/relocation handling, image orchestration, and link-stage planning.
- `src/*_tests.ds`: Dust test harness modules.
- `State.toml`: Dust workspace/sector metadata.

## Runtime Flow (Internal MVP)

1. `dust build src` emits a host executable.
2. `DustLinkMain::K::main` routes into `LinkerHostCli::K::run_with_host_args()`.
3. CLI parser resolves flags, input files, search paths, scripts, and output settings.
4. Link pipeline ingests object/archive members (ELF/COFF/Mach-O), resolves symbols/relocations, and emits final image.
5. Process exits with DustLink status code.

## Host Linker Selection During `dust build`

When the compiler links host executables:

- general host binaries: prefer `dustlink`, then try `lld` routes (`driver + -fuse-ld=lld`, `rust-lld`, `ld.lld`), then driver fallback.
- bootstrap build of `dustlink` executable itself: skip `dustlink` and prefer `lld`/`rust-lld` routes first.

## Dust Module Layer

The Dust modules provide deterministic linker behavior for:

- flag normalization and validation (gcc/ld/lld profile)
- target/format validation and routing
- target/emulation CLI mapping (`--target`, `-m`) into linker target IDs
- archive search (`.a`/`.lib`) over deterministic `-L` order
- archive search (`.a`/`.lib`) over deterministic explicit/default path order
  - explicit `-L` paths
  - `-rpath-link` paths in dynamic mode
  - target/sysroot default library roots
- dynamic-mode library search preferring shared objects before static archives
- static-mode library search constrained to static archives
- exact-name library token handling (`-l:<file>`)
- section and image planning helpers
- dynamic image-size planning for final emit
- symbol/relocation resolution and patch application
- symbol policy controls (`--defsym`, `--allow-multiple-definition`)
- required-symbol controls (`-u`, `--undefined`, `--require-defined`)
- dynamic unresolved-policy controls (`--no-undefined`, `--error-unresolved-symbols`, `--allow-shlib-undefined`)
- loader/search path controls (`--sysroot`, `-rpath`, `-rpath-link`)
- dynamic tag mode controls (`--enable-new-dtags`, `--disable-new-dtags`)
- transitive needed-entry controls (`--copy-dt-needed-entries`, `--no-copy-dt-needed-entries`)
- build-id controls (`--build-id` with mode/value)
- `-z` controls (`relro`/`norelro`, `now`/`lazy`, `execstack`/`noexecstack`)
- section GC policy controls (`--gc-sections`, `--no-gc-sections`)
- shared-symbol ingestion across ELF, PE, COFF, and Mach-O metadata paths
- script application (`ENTRY`, `OUTPUT`, `OUTPUT_FORMAT`, `OUTPUT_ARCH`, `TARGET`, `SEARCH_DIR`, `INPUT`, `GROUP`, `AS_NEEDED`, `NO_AS_NEEDED`, `EXTERN`, `PROVIDE`, `INCLUDE`, `MEMORY`, `SECTIONS` subset including location-counter and output-address forms)
- script `SEARCH_DIR(=...)` sysroot-aware resolution and script-token handling for `-L`/`-l` forms
- block-aware script statement splitting for multi-line linker scripts
- expanded relocation handling in Dust modules (`PLT32`, `GLOB_DAT`, `JUMP_SLOT`, `RELATIVE`, `GOTPCREL`, `GOTPCRELX`, `REX_GOTPCRELX`) and ELF machine validator acceptance for `EM_AARCH64`
- compatibility-state control wiring for hash/thread/eh-frame/diagnostic/print-gc/icf flags
- ELF staged writer flow now performs complete image emission with section-stream index validation on emit callbacks
- top-level orchestrated status/error flow in `DustLink::K`

`Q` and `Phi` remain domain-unavailable for operational paths and return `100`.
