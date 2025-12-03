#!/bin/bash
set -e

# This script converts system verilog files to verilog files in the ibex_gen_verilog of
# the current directory. It takes as input the ibex repository

if [[ -z "$1" ]]; then
    echo "Usage: $0 <path_to_repo>"
    exit 1
fi
repo=$1

rm -rf ./ibex_gen_verilog/
mkdir -p ./ibex_gen_verilog/

VENDOR_SV=(
    "$repo/vendor/lowrisc_ip/ip/prim_generic/rtl/prim_generic_buf.sv"
    "$repo/vendor/lowrisc_ip/ip/prim_generic/rtl/prim_generic_flop.sv"
)

# Convert dependency sources
for file in "${VENDOR_SV[@]}"; do
    module=$(basename -s .sv "$file")
    echo "Generating verilog for ${module} ..."

    sv2v \
        --define=SYNTHESIS --define=YOSYS \
        -I$repo/vendor/lowrisc_ip/ip/prim/rtl \
        "$file" \
        > ./ibex_gen_verilog/"${module}".v
done

# Generate vendor ram here as a pkg is needed
echo "Generating verilog for prim_generic_ram_2p ..."
sv2v --define=SYNTHESIS --define=YOSYS \
    -I$repo/vendor/lowrisc_ip/ip/prim/rtl \
    "$repo/vendor/lowrisc_ip/ip/prim/rtl/prim_ram_2p_pkg.sv" \
    "$repo/vendor/lowrisc_ip/ip/prim_generic/rtl/prim_generic_ram_2p.sv" \
    > ./ibex_gen_verilog/prim_generic_ram_2p.v

# Convert core sources
for file in $repo/rtl/*.sv \
    $repo/shared/rtl/*.sv  \
    $repo/examples/simple_system/rtl/*.sv; do
  module=$(basename -s .sv "$file")
  echo "Generating verilog for ${module} ..."

  # Skip packages
  if echo "$module" | grep -q '_pkg$'; then
      continue
  fi

  sv2v \
    --define=SYNTHESIS --define=YOSYS \
    $repo/rtl/*_pkg.sv \
    $repo/vendor/lowrisc_ip/ip/prim/rtl/prim_ram_1p_pkg.sv \
    $repo/vendor/lowrisc_ip/ip/prim/rtl/prim_secded_pkg.sv \
    -I$repo/vendor/lowrisc_ip/ip/prim/rtl \
    -I$repo/vendor/lowrisc_ip/dv/sv/dv_utils \
    "$file" \
    > ./ibex_gen_verilog/"${module}".v

  # Make sure auto-generated primitives are resolved to generic primitives
  # where available.
  sed -i 's/prim_buf/prim_generic_buf/g'  ./ibex_gen_verilog/"${module}".v
  sed -i 's/prim_flop/prim_generic_flop/g' ./ibex_gen_verilog/"${module}".v
  sed -i 's/prim_ram_2p/prim_generic_ram_2p/g' ./ibex_gen_verilog/"${module}".v
done

# Also add to our usable verilog files bare verilog helpers
cp $repo/syn/rtl/*.v ./ibex_gen_verilog/

# remove tracer (not needed for synthesis)
rm -f ./ibex_gen_verilog/ibex_tracer.v
rm -f ./ibex_gen_verilog/ibex_top_tracing.v

# Remove all register files, we will use the system verilog one
# directly from ibex_register_file_ff.sv
rm -f ./ibex_gen_verilog/ibex_register_file_latch.v
rm -f ./ibex_gen_verilog/ibex_register_file_fpga.v
rm -f ./ibex_gen_verilog/ibex_register_file_ff.v
cp $repo/rtl/ibex_register_file_ff.sv ./ibex_gen_verilog/
