set(target "prolead-aes_sbox_dom_d2")
set(generated_files cxxrtl_${target}.h cxxrtl_${target}.cpp ${target}.tsv)

file(GLOB HDL_SRC rtl/*.vhdl)
list(JOIN HDL_SRC " " HDL_STR)

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -m ${GHDL_MODULE} -p "ghdl ${HDL_STR} -e aes_sbox; hierarchy -check -top aes_sbox; rename aes_sbox top; ${YOSYS_COMMON_STEPS}; write_cxxrtl ${CXXRTL_OPTS} cxxrtl_${target}.cpp; write_table ${target}.tsv; stat;"
    DEPENDS ${HDL_SRC}
    VERBATIM
)
generate_accelerator(${target})
