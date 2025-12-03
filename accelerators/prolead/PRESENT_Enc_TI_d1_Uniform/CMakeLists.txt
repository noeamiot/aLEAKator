set(target "prolead-present_enc_ti_d1_uniform")
set(generated_files cxxrtl_${target}.h cxxrtl_${target}.cpp ${target}.tsv)

file(GLOB HDL_SRC rtl/*.vhd)
file(GLOB HDL_STR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} rtl/*.vhd)
list(JOIN HDL_STR_SRC " ${CMAKE_CURRENT_BINARY_DIR}/" HDL_STR)

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND mkdir -p rtl && cp ${HDL_SRC} rtl/
    COMMAND sed -rie "/UNISIM/d" rtl/g2.vhd rtl/g3.vhd rtl/Cterm.vhd
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -m ${GHDL_MODULE} -p "ghdl -fsynopsys ${HDL_STR} -e shared_present; hierarchy -check -top shared_present; rename shared_present top; ${YOSYS_COMMON_STEPS}; write_cxxrtl ${CXXRTL_OPTS} cxxrtl_${target}.cpp; write_table ${target}.tsv; stat;"
    DEPENDS ${HDL_SRC}
    BYPRODUCTS ./rtl/
    VERBATIM
)
generate_accelerator(${target})
