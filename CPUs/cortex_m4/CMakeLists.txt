set(cpu "cortex_m4")
set(generated_files cxxrtl_${cpu}.h cxxrtl_${cpu}.cpp ${cpu}.tsv)

# Generation of cxxrtl model is CPU-specific
file(GLOB_RECURSE dependencies rtl/*.v)

# If rtl directory is empty, ignore the whole CPU
list(LENGTH dependencies number_deps)
if (number_deps EQUAL 0)
    message(STATUS "Empty rtl sources for Cortex-m4, will not build this CPU.")
    return()
endif()

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/prepare.sh ${CMAKE_CURRENT_SOURCE_DIR}/rtl/
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -s ${CMAKE_CURRENT_SOURCE_DIR}/synth.ys
    COMMAND sed -ri "s/(.*p_uCORTEXM4INTEGRATION_2e_uCORTEXM4_2e_u__cm4__dpu_2e_u__cm4__dpu__regbank_2e_rf__pc__ex.symb_keep\\().*/\\1true\\);/g" cxxrtl_${cpu}.h
    COMMAND sed -ri "s/(.*p_uCORTEXM4INTEGRATION_2e_uCORTEXM4_2e_u__cm4__dpu_2e_u__cm4__dpu__dec_2e_instr__size__de.symb_keep\\().*/\\1true\\);/g" cxxrtl_${cpu}.h
    COMMAND sed -ri "s/(.*p_uCORTEXM4INTEGRATION_2e_uCORTEXM4_2e_u__cm4__dpu_2e_u__cm4__dpu__lsu_2e_u__cm4__dpu__lsu__ctl_2e_lss__pipe__adv__de.symb_keep\\().*/\\1true\\);/g" cxxrtl_${cpu}.h
    COMMAND sed -ri "s/(.*p_uCORTEXM4INTEGRATION_2e_uCORTEXM4_2e_u__cm4__dpu_2e_instr__de__2__ex__en.symb_keep\\().*/\\1true\\);/g" cxxrtl_${cpu}.h
    COMMAND patch cxxrtl_${cpu}.cpp ${CMAKE_CURRENT_SOURCE_DIR}/patch_cortex_m4.patch
    DEPENDS ${dependencies} prepare.sh synth.ys patch_cortex_m4.patch
    BYPRODUCTS ./cm4_gen_verilog/
    VERBATIM
)

# Generic configuration
set(target_opts --target=arm-none-eabi -mcpu=cortex-m4+nofp -nostdlib -ffreestanding -O3)
message(STATUS "Target flags set to: ${target_opts}")
gen_bootloader(${cpu})
# Generated programs is filled by gen_all_programs
gen_all_programs(${cpu} "${generated_programs}")
gen_cpu(${cpu} "${generated_programs}")
