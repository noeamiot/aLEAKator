set(cpu "aubrac")
set(configurations "unsecure;secure;secfast")

foreach(cfg IN LISTS configurations)
    set(generated_files "cxxrtl_${cpu}_${cfg}.h;cxxrtl_${cpu}_${cfg}.cpp;${cpu}_${cfg}.tsv")

    # Generation of cxxrtl model is CPU-specific
    file(GLOB_RECURSE dependencies rtl/*.sv)
    add_custom_command(
        OUTPUT ${generated_files}
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/prepare.sh ${CMAKE_CURRENT_SOURCE_DIR}/rtl/ ${cfg}
        COMMAND CONFIG=${cfg} ${YOSYS_CMD} ${YOSYS_OPTS} -c ${CMAKE_CURRENT_SOURCE_DIR}/synth.tcl
        DEPENDS ${dependencies} prepare.sh synth.tcl
        BYPRODUCTS ./aubrac_${cfg}_gen_verilog/
        VERBATIM
    )
endforeach()

# _program files are CPU-specific
function(gen_mem program target)
    add_custom_command(
        OUTPUT programs/${program}_program.cpp
        COMMAND ${CLANG_PATH}/bin/llvm-objcopy -O binary ${target} programs/${program}.bin
        COMMAND echo '\#include \"${program}_setup.h\"' > programs/${program}_program.cpp
        COMMAND echo '\#include \"aubrac_include.h\"' >> programs/${program}_program.cpp
        COMMAND echo 'void ${program}::load_implem\(cxxrtl_design::p_top& top\)' { >> programs/${program}_program.cpp
        COMMAND echo 'this->symbols\(\)\;' >> programs/${program}_program.cpp
        COMMAND od -A n -v -t x4 -w4 programs/${program}.bin | awk -v addr=0 '{
            print \"top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[\" addr \"] = value<32>{0x\" $$1 \"u}\;\"\; addr\+\=1;
        }' >> programs/${program}_program.cpp
        COMMAND echo "}" >> programs/${program}_program.cpp
        DEPENDS ${target} aubrac_include.h
        BYPRODUCTS programs/${program}.bin
    )
endfunction()

# Generic configuration
set(target_opts --target=riscv32 -march=rv32i -mabi=ilp32 -nostdlib -ffreestanding -O3)
message(STATUS "Target flags set to: ${target_opts}")
gen_bootloader(${cpu})
# Generated programs is filled by gen_all_programs
gen_all_programs(${cpu} "${generated_programs}")
gen_cpu(${cpu} "${generated_programs}" "${configurations}")
