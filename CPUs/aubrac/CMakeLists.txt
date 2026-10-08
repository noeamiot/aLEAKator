set(cpu "aubrac")
set(configurations "unsecure;secure;secfast")

file(GLOB_RECURSE dependency_check rtl/*.sv)
list(LENGTH dependency_check number_deps)
if (number_deps EQUAL 0)
    message(STATUS "Empty rtl sources for Aubrac, will not build this CPU.")
    return()
endif()

foreach(cfg IN LISTS configurations)
    set(generated_files "cxxrtl_${cpu}_${cfg}.h;cxxrtl_${cpu}_${cfg}.cpp;${cpu}_${cfg}.tsv")

    # Generation of cxxrtl model is CPU-specific
    file(GLOB_RECURSE dependencies rtl/*.sv)
    add_custom_command(
        OUTPUT ${generated_files}
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/prepare.sh ${CMAKE_CURRENT_SOURCE_DIR}/rtl/ ${cfg}
        COMMAND CONFIG=${cfg} ${YOSYS_CMD} ${YOSYS_OPTS} --logfile synth_${cfg}.log -c ${CMAKE_CURRENT_SOURCE_DIR}/synth.tcl
        DEPENDS ${dependencies} prepare.sh synth.tcl
        BYPRODUCTS ./aubrac_${cfg}_gen_verilog/
        VERBATIM
    )
endforeach()

# Generic configuration
set(target_opts --target=riscv32 -march=rv32i -mabi=ilp32 -nostdlib -ffreestanding -O3)
message(STATUS "Target flags set to: ${target_opts}")
gen_bootloader(${cpu})
# Generated programs is filled by gen_all_programs
gen_all_programs(${cpu} "${generated_programs}")
gen_cpu(${cpu} "${generated_programs}" "${configurations}")
