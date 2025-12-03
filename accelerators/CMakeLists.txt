function(generate_accelerator name)
    message(STATUS "Adding accelerator: ${name}")
    add_executable(${name})
    target_sources(${name}
        PRIVATE
        simulate.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/cxxrtl_${name}.cpp
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/cxxrtl_${name}.h
    )
    target_include_directories(${name}
        PUBLIC
        ${CXXRTL_PATH}
        ${VERIFMSI_PATH}
        ${LSS_PATH}
        ${ALEAKATOR_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_link_libraries(${name} PUBLIC ${Boost_LIBRARIES} lss aleakator verif_msi_pp)
endfunction()

# Build all accelerators projects
file(GLOB SUBDIRS LIST_DIRECTORIES YES CONFIGURE_DEPENDS "*/*")
foreach(subdir ${SUBDIRS})
    # Exclude submodule directory
    if(EXISTS ${subdir}/CMakeLists.txt)
        add_subdirectory(${subdir})
    endif()
endforeach()
