#!/bin/bash
set -e

# This script simply copies the verilog sources of the cortex core to the cm3_gen_verilog
# as it is easier to handle it this way in yosys

if [[ -z "$1" ]]; then
    echo "Usage: $0 <path_to_repo>"
    exit 1
fi
repo=$1

rm -rf ./cm3_gen_verilog/
mkdir -p ./cm3_gen_verilog/

# Read full CPU sources
cp $repo/logical/cm3_bus_matrix/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_dap_ahb_ap/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_dpu/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_dwt/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_etm/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_fpb/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_itm/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_lic_defs/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_mpu/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_nvic/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_tpiu/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cm3_wic/verilog/*.v ./cm3_gen_verilog/
cp $repo/logical/cortexm3/verilog/*.v ./cm3_gen_verilog/

cp $repo/logical/cortexm3_integration/verilog/*.v ./cm3_gen_verilog/

cp $repo/logical/models/cells/*.v ./cm3_gen_verilog/
cp $repo/logical/dapswjdp/verilog/*.v ./cm3_gen_verilog/

# Read other SOC ressources
cp $repo/example/tbench/example_tbench.v ./cm3_gen_verilog/
cp $repo/example/tbench/SinglePortRAM.v ./cm3_gen_verilog/
cp $repo/example/tbench/DualPortRAM.v ./cm3_gen_verilog/
cp $repo/example/tbench/DefaultSlave.v ./cm3_gen_verilog/

cp $repo/example/tbench/CM3ExampleConfig.vh ./cm3_gen_verilog/
