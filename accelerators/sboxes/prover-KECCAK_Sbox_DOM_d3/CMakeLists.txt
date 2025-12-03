set(target "prover-keccak_sbox_dom_d3")
set(generated_files cxxrtl_${target}.h cxxrtl_${target}.cpp ${target}.tsv)

file(GLOB HDL_SRC rtl/*.v)
list(JOIN HDL_SRC " " HDL_STR)

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -m ${GHDL_MODULE} -p "read_verilog -sv ${HDL_STR}; hierarchy -check -top keccak_sbox; rename keccak_sbox top; ${YOSYS_COMMON_STEPS}; write_cxxrtl ${CXXRTL_OPTS} cxxrtl_${target}.cpp; write_table ${target}.tsv; stat;"
    DEPENDS ${HDL_SRC}
    VERBATIM
)
generate_accelerator(${target})
