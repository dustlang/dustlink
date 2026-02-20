# Getting Started

## Prerequisites

- Dust compiler (`dust`) built and available.
- For host executable linking in current compiler stage, have one of:
  - `dustlink` in `PATH` (preferred for non-bootstrap builds),
  - `lld` support via compiler driver (`clang`/`cc` with `-fuse-ld=lld`),
  - `rust-lld`.

## Build

From `dustlink/`:

```bash
dust build src --out target/dust/dustlink
```

Binary output (Windows):

```text
target\dust\dustlink.exe
```

## Basic Usage

Display help:

```bash
dustlink --help
```

Show version:

```bash
dustlink --version
```

Link objects to ELF:

```bash
dustlink -o kernel.bin --entry 0x100000 obj1.o obj2.o
```

Link to PE:

```bash
dustlink --oformat=pe -o kernel.exe obj1.o obj2.o
```

Apply a linker script:

```bash
dustlink -T link.ld -o kernel.bin obj1.o obj2.o
```

Emit map file:

```bash
dustlink -Map kernel.map -o kernel.bin obj1.o obj2.o
```

`dustlink` is internal-linker mode and does not forward to external linker backends.
