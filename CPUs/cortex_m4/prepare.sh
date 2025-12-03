#!/bin/bash
set -e

# This script simply copies the verilog sources of the cortex core to the cm4_gen_verilog
# as it is easier to handle it this way in yosys

if [[ -z "$1" ]]; then
    echo "Usage: $0 <path_to_repo>"
    exit 1
fi
repo=$1

rm -rf ./cm4_gen_verilog/
mkdir -p ./cm4_gen_verilog/

# Read full CPU sources
cp $repo/logical/cm4_bus_matrix/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_dap_ahb_ap/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_dpu/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_dwt/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_fpb/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_itm/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_lic_defs/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_mpu/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_nvic/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_tpiu/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cm4_wic/verilog/*.v ./cm4_gen_verilog/
cp $repo/logical/cortexm4/verilog/*.v ./cm4_gen_verilog/

cp $repo/logical/cortexm4_integration/verilog/*.v ./cm4_gen_verilog/

cp $repo/logical/models/cells/*.v ./cm4_gen_verilog/
cp $repo/logical/dapswjdp/verilog/*.v ./cm4_gen_verilog/

# Read other SOC ressources
cp $repo/example/tbench/example_tbench.v ./cm4_gen_verilog/
cp $repo/example/tbench/SinglePortRAM.v ./cm4_gen_verilog/
cp $repo/example/tbench/DualPortRAM.v ./cm4_gen_verilog/
cp $repo/example/tbench/DefaultSlave.v ./cm4_gen_verilog/

cp $repo/example/tbench/CM3ExampleConfig.vh ./cm4_gen_verilog/
