#!/bin/bash
set -e

# This script converts system verilog files to verilog files in the cv32e40p_gen_verilog of
# the current directory. It takes as input the cv32e40p repository

if [[ -z "$1" ]]; then
    echo "Usage: $0 <path_to_repo>"
    exit 1
fi
repo=$1

rm -rf ./cv32e40p_gen_verilog/
mkdir -p ./cv32e40p_gen_verilog/

# Generate vendor ram here as a pkg is needed
echo "Generating verilog for prim_generic_ram_2p ..."
sv2v --define=SYNTHESIS --define=YOSYS \
    -I$repo/example_tb/simple_system/prim_import/ \
    $repo/example_tb/simple_system/prim_import/prim_ram_2p_pkg.sv \
    $repo/example_tb/simple_system/prim_generic_ram_2p.sv  \
    > ./cv32e40p_gen_verilog/prim_generic_ram_2p.v

# Convert cv32e40p core
for file in $repo/rtl/*.sv; do
  module=`basename -s .sv $file`
  echo "Generating verilog for $module" ;
  sv2v \
    --define=SYNTHESIS \
    $repo/rtl/include/*_pkg.sv \
    $repo/rtl/vendor/pulp_platform_fpnew/src/fpnew_pkg.sv \
    $file \
    > ./cv32e40p_gen_verilog/${module}.v
done

# Convert ibex shared elements
for file in $repo/example_tb/simple_system/shared_ibex/*.sv; do
  module=$(basename -s .sv "$file")
  echo "Generating verilog for ${module} ..."

  sv2v \
    --define=SYNTHESIS --define=YOSYS \
    -I$repo/example_tb/simple_system/prim_import/ \
    "$file" > ./cv32e40p_gen_verilog/"${module}".v

  # Make sure auto-generated primitives are resolved to generic primitives
  # where available.
  sed -i 's/prim_ram_2p/prim_generic_ram_2p/g' ./cv32e40p_gen_verilog/"${module}".v
done
sv2v $repo/example_tb/simple_system/simple_system.sv > ./cv32e40p_gen_verilog/simple_system.v
sv2v $repo/bhv/cv32e40p_sim_clock_gate.sv > ./cv32e40p_gen_verilog/cv32e40p_sim_clock_gate.v


# remove the latch-based register file (because we will use the
# register-based one instead)
rm -f ./cv32e40p_gen_verilog/cv32e40p_register_file_latch.v
