set(cpu "coco-ibex")
set(generated_files cxxrtl_${cpu}.h cxxrtl_${cpu}.cpp ${cpu}.tsv)

# Generation of cxxrtl model is CPU-specific
file(GLOB_RECURSE dependencies rtl/*.sv)
add_custom_command(
    OUTPUT ${generated_files}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/prepare.sh ${CMAKE_CURRENT_SOURCE_DIR}/rtl/
    COMMAND ${YOSYS_CMD} ${YOSYS_OPTS} -s ${CMAKE_CURRENT_SOURCE_DIR}/synth.ys
    COMMAND sed -ri "s/(.*p_u__core_2e_data__addr__o.symb_keep\\().*/\\1true\\);/g" cxxrtl_${cpu}.h
    COMMAND sed -ri "s/(.*p_u__core_2e_data__req__o.symb_keep\\().*/\\1true\\);/g" cxxrtl_${cpu}.h
    COMMAND sed -ri "s/(.*p_u__core_2e_data__we__o.symb_keep\\().*/\\1true\\);/g" cxxrtl_${cpu}.h
    DEPENDS ${dependencies} prepare.sh synth.ys
    BYPRODUCTS ./ibex_gen_verilog/
    VERBATIM
)

# Generic configuration
set(target_opts --target=riscv32 -march=rv32ic -mabi=ilp32 -nostdlib -ffreestanding -Xlinker --no-check-sections -O3)
message(STATUS "Target flags set to: ${target_opts}")
gen_bootloader(${cpu})
# Generated programs is filled by gen_all_programs
gen_all_programs(${cpu} "${generated_programs}")
gen_cpu(${cpu} "${generated_programs}")
