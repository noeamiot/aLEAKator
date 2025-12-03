#!/bin/bash
set -e

# This script converts system verilog files to verilog files in the ibex_gen_verilog of
# the current directory. It takes as input the ibex repository

if [[ -z "$1" ]]; then
    echo "Usage: $0 <path_to_repo>"
    exit 1
fi
repo=$1

rm -rf ./coco-ibex_gen_verilog/
mkdir -p ./coco-ibex_gen_verilog/

for file in $repo/rtl/*.sv; do
  module=`basename -s .sv $file`
  echo "Translating module $module" ;
  sv2v \
    --define=SYNTHESIS \
    $repo/rtl/*_pkg.sv \
    -I$repo/shared/rtl \
    $file \
    > ./coco-ibex_gen_verilog/${module}.v
    sed -ie "s/ *if ([a-zA-Z0-9]*) ;//g" ./coco-ibex_gen_verilog/${module}.v
done

cp $repo/shared/rtl/ram_1p_secure.v ./coco-ibex_gen_verilog/
cp $repo/shared/rtl/rom_1p.v ./coco-ibex_gen_verilog/
cp $repo/shared/rtl/ibex_top.v ./coco-ibex_gen_verilog/
# Hardcoded path are an issue
sed -ri "s/..\/..\/rtl\/secure.sv/secure.sv/g" coco-ibex_gen_verilog/ibex_top.v
sed -ri "s/\`include \"r.*\.v\"//g" coco-ibex_gen_verilog/ibex_top.v
sed -ri "s/..\/..\/rtl\/secure.sv/secure.sv/g" coco-ibex_gen_verilog/ibex_top.v

# remove generated *pkg.v files (they are empty files and not needed)
rm -f ./coco-ibex_gen_verilog/*_pkg.v

# remove tracer (not needed for synthesis)
rm -f ./coco-ibex_gen_verilog/ibex_tracer.v
rm -r ./coco-ibex_gen_verilog/ibex_core_tracing.v
cp $repo/rtl/secure.sv ./coco-ibex_gen_verilog/

# remove the FPGA & latch-based register file (because we will use the
# ff-based one instead)
rm -f ./coco-ibex_gen_verilog/ibex_register_file_latch.v
rm -f ./coco-ibex_gen_verilog/ibex_register_file_fpga.v
