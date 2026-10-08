set(cpu "ibex")
set(generated_files cxxrtl_${cpu}.h cxxrtl_${cpu}.cpp ${cpu}.tsv)

# Generation of cxxrtl model is CPU-specific
file(GLOB_RECURSE dependencies rtl/*.sv)
add_custom_command(
    OUTPUT ${generated_files}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/prepare.sh ${CMAKE_CURRENT_SOURCE_DIR}/rtl/
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -s ${CMAKE_CURRENT_SOURCE_DIR}/synth.ys
    COMMAND sed -ri "s/(.*p_u__top_2e_u__ibex__core_2e_cs__registers__i_2e_pc__id__i.symb_keep\\().*/\\1true\\);/g" cxxrtl_${cpu}.h
    COMMAND patch cxxrtl_${cpu}.cpp ${CMAKE_CURRENT_SOURCE_DIR}/patch_ibex.patch
    DEPENDS ${dependencies} prepare.sh synth.ys patch_ibex.patch
    BYPRODUCTS ./ibex_gen_verilog/
    VERBATIM
)

# Generic configuration
set(target_opts --target=riscv32 -march=rv32imc -mabi=ilp32 -nostdlib -ffreestanding -O3)
message(STATUS "Target flags set to: ${target_opts}")
gen_bootloader(${cpu})
# Generated programs is filled by gen_all_programs
gen_all_programs(${cpu} "${generated_programs}")
gen_cpu(${cpu} "${generated_programs}")
