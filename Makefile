# DustLink standalone build
#
# Usage:
#   make              - Build dustlink (requires `dust` compiler in PATH or DUST env)
#   make clean        - Remove build artifacts
#   make CC=clang     - Use a different C compiler
#
# Prerequisites:
#   - dust compiler (from dust/ or dust-bootstrap/)
#   - cc (system C compiler for host runtime shim)

DUST   ?= dust
CC     ?= cc
CFLAGS := -Wall -Wno-unused-function -Wno-unused-parameter -Wno-sign-compare -O2

SRCDIR := src
OUTDIR := target
OUT    := $(OUTDIR)/dustlink

SHIM_SRCS  := $(SRCDIR)/hostlinker_shim.c
SHIM_OBJ   := $(OUTDIR)/hostlinker_shim.o
DUST_SRCS  := $(wildcard $(SRCDIR)/*.ds)
DUST_OBJ   := $(OUTDIR)/dustlink_core.o

.PHONY: all clean

all: $(OUT)

$(OUTDIR):
	mkdir -p $(OUTDIR)

# Step 1: Compile C host runtime shim
$(SHIM_OBJ): $(SHIM_SRCS) | $(OUTDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Step 2: Compile Dust sources to a single object file
$(DUST_OBJ): $(DUST_SRCS) | $(OUTDIR)
	$(DUST) obj $(SRCDIR) --out $@

# Step 3: Link with system linker (cc)
$(OUT): $(SHIM_OBJ) $(DUST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -rf $(OUTDIR)
