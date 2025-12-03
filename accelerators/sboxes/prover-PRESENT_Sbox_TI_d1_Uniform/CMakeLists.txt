set(target "prover-present_sbox_ti_d1_uniform")
set(generated_files cxxrtl_${target}.h cxxrtl_${target}.cpp ${target}.tsv)

file(GLOB HDL_SRC rtl/*.vhd)
list(JOIN HDL_SRC " " HDL_STR)

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -m ${GHDL_MODULE} -p "ghdl -fsynopsys ${HDL_STR} -e sharedSbox; hierarchy -check -top sharedSbox; rename sharedSbox top; ${YOSYS_COMMON_STEPS}; write_cxxrtl ${CXXRTL_OPTS} cxxrtl_${target}.cpp; write_table ${target}.tsv; stat;"
    DEPENDS ${HDL_SRC}
    VERBATIM
)
generate_accelerator(${target})
