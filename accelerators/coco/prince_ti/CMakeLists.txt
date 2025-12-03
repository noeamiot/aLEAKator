set(target "prince_ti")
set(generated_files cxxrtl_${target}.h cxxrtl_${target}.cpp ${target}.tsv)

file(GLOB HDL_SRC rtl/*.sv)
list(FILTER HDL_SRC EXCLUDE REGEX .*top_module_d11_secure.*)
list(JOIN HDL_SRC " " HDL_STR)
#rtl/top_module_d11.sv rtl/affine_2sh.sv rtl/mprime_cir_2sh.sv rtl/s_bit1_sh3.sv rtl/s_bit2_sh1.sv rtl/s_bit2_sh8.sv rtl/s_bit3_sh6.sv rtl/s_bit4_sh4.sv rtl/share_reg.sv rtl/xor_cir4.sv rtl/affine_share1.sv rtl/Mprime_cir.sv rtl/s_bit1_sh4.sv rtl/s_bit2_sh2.sv rtl/s_bit2.sv rtl/s_bit3_sh7.sv rtl/s_bit4_sh5.sv rtl/sr_2sh.sv rtl/xor_cir.sv rtl/affine_share2.sv rtl/mux_cir_2sh.sv rtl/s_bit1_sh5.sv rtl/s_bit2_sh3.sv rtl/s_bit3_sh1.sv rtl/s_bit3_sh8.sv rtl/s_bit4_sh6.sv rtl/sr_cir.sv rtl/inv_sr_2sh.sv rtl/mux_cir.sv rtl/s_bit1_sh6.sv rtl/s_bit2_sh4.sv rtl/s_bit3_sh2.sv rtl/s_bit3.sv rtl/s_bit4_sh7.sv rtl/s.sv rtl/inv_sr_cir.sv rtl/out_key_share_flip.sv rtl/s_bit1_sh7.sv rtl/s_bit2_sh5.sv rtl/s_bit3_sh3.sv rtl/s_bit4_sh1.sv rtl/s_bit4_sh8.sv rtl/M0_Share1.sv rtl/s_bit1_sh1.sv rtl/s_bit1_sh8.sv rtl/s_bit2_sh6.sv rtl/s_bit3_sh4.sv rtl/s_bit4_sh2.sv rtl/s_bit4.sv rtl/M1_Share1.sv rtl/s_bit1_sh2.sv rtl/s_bit1.sv rtl/s_bit2_sh7.sv rtl/s_bit3_sh5.sv rtl/s_bit4_sh3.sv rtl/sbox_rmsk.sv rtl/xor_cir_2sh.sv 

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -p "read_verilog -sv ${HDL_STR}; hierarchy -check -top top_module_d11; rename top_module_d11 top; ${YOSYS_COMMON_STEPS}; write_cxxrtl ${CXXRTL_OPTS} cxxrtl_${target}.cpp; write_table ${target}.tsv; stat;"
    DEPENDS ${HDL_SRC}
    VERBATIM
)
generate_accelerator(${target})
