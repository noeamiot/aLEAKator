#!/bin/bash
set -e

# This script converts system verilog files to verilog files in the aubrac_gen_verilog of
# the current directory. It takes as input the aubrac repository

if [[ -z "$1" || -z "$2" ]]; then
    echo "Usage: $0 <path_to_repo> <config>"
    exit 1
fi
repo=$1

version="WAESP32AU1V010Core"
if [[ $2 == "secure" ]]; then
    version="WAESP32AU1V010S0Core"
elif [[ $2 == "secfast" ]]; then
    version="WAESP32AU1V010S1Core"
fi

rm -rf ./aubrac_${2}_gen_verilog/
mkdir -p ./aubrac_${2}_gen_verilog/

# Convert core sources
for file in $repo/$version/src/*.sv \
    $repo/simple_system/*.sv; do
  module=$(basename -s .sv "$file")
  echo "Generating verilog for ${module} ..."

  # Skip packages
  if echo "$module" | grep -q '_pkg$'; then
      continue
  fi

  sv2v \
    --define=SYNTHESIS --define=YOSYS \
    "$file" \
    > ./aubrac_${2}_gen_verilog/"${module}".v
done
