function(gen_utest directory)
    get_filename_component(name ${directory} NAME)
        message( STATUS "Adding utest ${name}")
    add_executable(${name})
    target_sources(${name}
        PRIVATE
        ${directory}/simulate.cpp
    )
    target_include_directories(${name}
        PUBLIC
        ${CXXRTL_PATH}
        ${VERIFMSI_PATH}
        ${LSS_PATH}
        ${ALEAKATOR_PATH}
    )
    target_link_libraries(${name} PUBLIC lss aleakator verif_msi_pp)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# Build all utests
file(GLOB utest_dirs LIST_DIRECTORIES YES CONFIGURE_DEPENDS "*/*")
foreach(subdir ${utest_dirs})
    if (IS_DIRECTORY ${subdir})
        gen_utest(${subdir})
    endif()
endforeach()
