# Generate all targets for a program
function(gen_program cpu program)
    set(program_path ${CMAKE_CURRENT_SOURCE_DIR}/programs/${program}/)
    set(target ${cpu}-${program})
    add_executable(${target})

    # Depending on the form of the program, set different sources
    if (EXISTS ${program_path}/CMakeLists.txt)
        # For program with custom rules, call its cmake file, which must create
        # the relevant .o file
        message(DEBUG "Using custom source for ${program}")
        add_subdirectory(${program_path})
        #configure_file(${CMAKE_CURRENT_BINARY_DIR}/programs/${program}/${program}.o programs/${program}.o COPYONLY)
    elseif (EXISTS ${program_path}/${program}.o)
        message(DEBUG "Using .o source for ${program}")
        configure_file(${program_path}/${program}.o programs/${program}.o COPYONLY)
    elseif(EXISTS ${program_path}/${program}.S)
        message(DEBUG "Using .S source for ${program}")
        add_custom_command(
            OUTPUT programs/${program}.o
            COMMAND ${CMAKE_C_COMPILER} ${target_opts} -c -o programs/${program}.o ${program_path}/${program}.S
            DEPENDS ${program_path}/${program}.S
            VERBATIM
        )
        configure_file(${program_path}/${program}.S programs/${program}.S COPYONLY)
    elseif(EXISTS ${program_path}/${program}.c)
        message(DEBUG "Using .c source for ${program}")
        add_custom_command(
            OUTPUT programs/${program}.o
            COMMAND ${CMAKE_C_COMPILER} ${target_opts} -c -o programs/${program}.o ${program_path}/${program}.c
            DEPENDS ${program_path}/${program}.c
            VERBATIM
        )
        configure_file(${program_path}/${program}.c programs/${program}.c COPYONLY)
    else()
        message( SEND_ERROR "Program must be either .o, .S, .c or have a CMakeLists.txt" )
    endif()

    target_sources(${target}
        PRIVATE
        programs/startup.o
        programs/${program}.o
    )
    target_link_options(${target} PRIVATE -v -fuse-ld=lld ${target_opts} -T ${CMAKE_CURRENT_SOURCE_DIR}/programs/link.ld)
    target_compile_options(${target} PRIVATE ${target_opts})
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE C)

    add_custom_command(
        OUTPUT programs/${program}_symbols.cpp
        COMMAND echo '\#include \"${program}_setup.h\"' > programs/${program}_symbols.cpp
        COMMAND echo 'void ${program}::symbols_implem\(\)' { >> programs/${program}_symbols.cpp
        COMMAND ${CLANG_PATH}/bin/llvm-nm --print-size ${target} | while read -r addr size type name\; do echo \"symbols_[\\\"$$name\\\"] = {0x$$addr, 0x$$size}\;\"\; done >> programs/${program}_symbols.cpp
        COMMAND echo "}" >> programs/${program}_symbols.cpp
        DEPENDS ${target}
    )
    gen_mem(${program} ${target})
endfunction()

function(gen_bootloader cpu)
    add_custom_command(
        OUTPUT programs/startup.o
        COMMAND ${CMAKE_C_COMPILER} ${target_opts} -c -o programs/startup.o ${CMAKE_CURRENT_SOURCE_DIR}/programs/startup.S
        DEPENDS programs/startup.S
        VERBATIM
    )
endfunction()

# Loop through the program folder subdirectories and call gen_program for each
function(gen_all_programs cpu generated_programs)
    set(generated_programs "")
    file(GLOB programs programs/* LIST_DIRECTORIES YES)
    foreach(program_path ${programs})
        file(GLOB program_setup_file ${program_path}/*_setup.cpp)
        if(EXISTS ${program_setup_file})
            get_filename_component(program ${program_path} NAME)
            message(STATUS "Adding CPU program: ${program}")
            gen_program(${cpu} ${program})
            list(APPEND generated_programs ${program})
        endif()
    endforeach()

    string(REGEX REPLACE "([^;]+)" "#include\"\\1_setup.h\"" includes "${generated_programs}")
    string(REGEX REPLACE "([^;]+)" "programs[\"\\1\"]=std::make_unique<\\1>()" instances "${generated_programs}")
    string(REGEX REPLACE "([^;]+)" "programs/\\1_program.cpp;programs/\\1_symbols.cpp" progs "${generated_programs}")
    list(JOIN includes "\\n" includes)
    list(JOIN instances "\;\\n" instances)
    add_custom_command(
        OUTPUT programs_include.h programs_instance.h
        COMMAND /bin/echo -e \'${includes}\' > programs_include.h
        COMMAND /bin/echo -e \'${instances}\;\' > programs_instance.h
        DEPENDS
    )
    add_custom_target(${cpu}_all_programs DEPENDS programs_include.h programs_instance.h ${progs})
    return(PROPAGATE cpu generated_programs)
endfunction()

# There is a third, optionnal argument which is a list of configs
function(gen_cpu cpu generated_programs)
        string(REGEX REPLACE "([^;]+)" "programs/\\1/\\1_setup.cpp;${CMAKE_CURRENT_BINARY_DIR}/programs/\\1_program.cpp;${CMAKE_CURRENT_BINARY_DIR}/programs/\\1_symbols.cpp" generated_load_symb "${generated_programs}")
        string(REGEX REPLACE "([^;]+)" "programs/\\1/\\1_setup.h" programs_headers "${generated_programs}")
        string(REGEX REPLACE "([^;]+)" "programs/\\1/" programs_dirs "${generated_programs}")

        if(ARGC LESS 3)
            set(configs "_base")
        else()
            set(configs ${ARGV2})
            list(TRANSFORM configs PREPEND "_")
        endif()

        foreach(cfg IN LISTS configs)
            message(STATUS "Adding config ${cfg} for CPU ${cpu}")
            if (cfg STREQUAL "_base")
                set(cfg "")
            endif()

            add_executable(${cpu}${cfg})
            target_compile_definitions(${cpu}${cfg} PRIVATE CFG${cfg})
            target_sources(${cpu}${cfg}
                PRIVATE
                simulate.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/cxxrtl_${cpu}${cfg}.cpp
                ${generated_load_symb}
                PUBLIC
                ${CMAKE_CURRENT_BINARY_DIR}/cxxrtl_${cpu}${cfg}.h
                ${CMAKE_CURRENT_BINARY_DIR}/programs_include.h
                ${CMAKE_CURRENT_BINARY_DIR}/programs_instance.h
                ${programs_headers}
                ./${cpu}_program.h
                ../program_setup.h
            )
            target_include_directories(${cpu}${cfg}
                PUBLIC
                ${CXXRTL_PATH}
                ${VERIFMSI_PATH}
                ${LSS_PATH}
                ${ALEAKATOR_PATH}
                ${CMAKE_CURRENT_BINARY_DIR}
                ${programs_dirs}
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/../
            )
            add_dependencies(${cpu}${cfg} ${cpu}_all_programs)
            target_link_libraries(${cpu}${cfg} PUBLIC ${Boost_LIBRARIES} lss aleakator verif_msi_pp)
    endforeach()
endfunction()

# Loop through all the CPUs
file(GLOB SUBDIRS LIST_DIRECTORIES YES CONFIGURE_DEPENDS *)
foreach(subdir ${SUBDIRS})
    # Exclude submodule directory
    if(EXISTS ${subdir}/CMakeLists.txt)
        get_filename_component(cpu ${subdir} NAME)
        message(STATUS "Adding CPU ${cpu}")
        add_subdirectory(${cpu})
    endif()
endforeach()
