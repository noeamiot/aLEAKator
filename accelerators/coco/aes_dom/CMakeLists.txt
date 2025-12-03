set(target "aes_dom")
set(generated_files cxxrtl_${target}.h cxxrtl_${target}.cpp ${target}.tsv)

file(GLOB HDL_SRC rtl/*.vhdl)
list(JOIN HDL_SRC " " HDL_STR)
#    rtl/aes_ctrl_lsfr_23_cycles.vhdl rtl/aes_sbox.vhdl rtl/gf2_mul.vhdl rtl/masked_aes_pkg.vhdl rtl/rcon.vhdl rtl/shared_mul_gf2.vhdl rtl/square_scaler.vhdl rtl/aes_ctrl_lsfr.vhdl rtl/aes_state_regs.vhdl rtl/inverter.vhdl rtl/masked_mul.vhdl rtl/real_dom_shared_mul_gf2.vhdl rtl/shared_mul_gf4.vhdl rtl/aes_key_regs.vhdl rtl/aes_top.vhdl rtl/lin_map.vhdl rtl/mix_columns.vhdl rtl/real_dom_shared_mul_gf4.vhdl rtl/shared_mul.vhdl

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -m ${GHDL_MODULE} -p "ghdl ${HDL_STR} -e aes_top; hierarchy -check -top aes_top; rename aes_top top; ${YOSYS_COMMON_STEPS}; write_cxxrtl ${CXXRTL_OPTS} cxxrtl_${target}.cpp; write_table ${target}.tsv; stat;"
    DEPENDS ${HDL_SRC}
    VERBATIM
)
generate_accelerator(${target})
