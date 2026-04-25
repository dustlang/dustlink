# DustLink standalone build
#
# Usage:
#   make              - Compile C shim and Dust sources
#   make link         - Link dustlink executable
#   make clean        - Remove build artifacts
#
# Prerequisites:
#   - dust compiler (from dust/ or dust-bootstrap/)
#   - cc (system C compiler for host runtime shim)

DUST ?= dust
DUST_PATH := /home/andres/dust.llc/code/dustlang/dust/target/release/dust
CC ?= cc
CFLAGS := -Wall -Wno-unused-function -Wno-unused-parameter -Wno-sign-compare -O2

SRCDIR := src
OUTDIR := target
OBJDIR := $(OUTDIR)/dustobj

SHIM_SRCS := $(SRCDIR)/hostlinker_shim.c
SHIM_OBJ := $(OUTDIR)/hostlinker_shim.o
SHIM_HDR := $(SRCDIR)/hostlinker_shim.h

.PHONY: all clean compile-shim compile-dust link help

all: compile-shim compile-dust
	@echo ""
	@echo "Phase 2 complete: C shim and Dust sources compiled"
	@echo "Next: make link to create executable, or use dust build directly"

compile-shim: $(SHIM_OBJ)
	@echo "C shim compiled successfully"

$(SHIM_OBJ): $(SHIM_SRCS) $(SHIM_HDR)
	@mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) -c $< -o $@

compile-dust:
	@mkdir -p $(OBJDIR)
	@echo "Compiling Dust sources to $(OBJDIR)..."
	$(DUST_PATH) obj $(SRCDIR) --out-dir $(OBJDIR) --skip-tests
	@echo "Dust compilation complete"
	@ls -la $(OBJDIR)/

clean:
	rm -rf $(OUTDIR)

help:
	@echo "DustLink Build Targets:"
	@echo "  make compile-shim   - Compile C FFI shim (hostlinker_shim.c)"
	@echo "  make compile-dust   - Compile Dust sources to object files"
	@echo "  make all            - Compile both C and Dust"
	@echo "  make link           - Link dustlink executable (not implemented)"
	@echo "  make clean          - Remove build artifacts"