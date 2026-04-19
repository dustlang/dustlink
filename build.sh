#!/bin/bash
set -e

DUST="/home/andres/dust.llc/code/dustlang/dust/target/release/dust"
CC="cc"
OUTDIR="target"
OBJDIR="$OUTDIR/dustobj"

rm -rf "$OBJDIR" "$OUTDIR/dustlink_core.o"
mkdir -p "$OBJDIR" "$OUTDIR"

# Step 1: Compile each .ds file (excluding tests)
echo "Compiling dustlink.dust..."
for file in src/*.ds; do
  if [[ "$file" == *"_tests.ds" ]]; then
    continue
  fi
  echo "  $file"
  "$DUST" obj "$file" --out-dir "$OBJDIR" --target x86_64-unknown-linux-gnu || {
    echo "Failed: $file"
    exit 1
  }
done

echo "Compilation completed"
ls -la "$OBJDIR/"
