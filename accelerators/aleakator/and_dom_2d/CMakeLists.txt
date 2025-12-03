set(target "and_dom_2d")
set(generated_files cxxrtl_${target}.h cxxrtl_${target}.cpp ${target}.tsv)

set(HDL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/and_dom_2d.v")
set(HDL_STR ${HDL_SRC})

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -p "read_verilog -sv ${HDL_STR}; hierarchy -check -top top; ${YOSYS_COMMON_STEPS}; write_cxxrtl ${CXXRTL_OPTS} cxxrtl_${target}.cpp; write_table ${target}.tsv; stat;"
    DEPENDS ${HDL_SRC}
    VERBATIM
)
generate_accelerator(${target})
