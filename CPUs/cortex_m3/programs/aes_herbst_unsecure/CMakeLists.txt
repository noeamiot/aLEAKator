# Rules to create the aes_herbst_unsecure.o file
set(program "aes_herbst_unsecure")
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../${program}.o
    COMMAND ${CMAKE_C_COMPILER} ${target_opts} -S -o ${program}.S ${CMAKE_CURRENT_SOURCE_DIR}/${program}.c
    COMMAND sed -i -f ${CMAKE_CURRENT_SOURCE_DIR}/../remove_s_instructions.sed ${program}.S
    COMMAND patch ${program}.S ${CMAKE_CURRENT_SOURCE_DIR}/${program}.patch
    COMMAND ${CMAKE_C_COMPILER} ${target_opts} -c -o ${CMAKE_CURRENT_BINARY_DIR}/../${program}.o ${program}.S
    DEPENDS aes_herbst_unsecure.c ../remove_s_instructions.sed ${program}.patch
    BYPRODUCTS aes_herbst_unsecure.S
    VERBATIM
)

add_custom_target(${target}_gen_bin DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../${program}.o)
add_dependencies(${target} ${target}_gen_bin)
