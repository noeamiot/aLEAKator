# Build the lss and aleakator libraries
set(LSS_SOURCES ${LSS_PATH}/lss.cpp)

add_library(lss STATIC)
target_sources(lss
    PRIVATE
    ${LSS_SOURCES}
    PUBLIC
    ${VERIF_MSI_HEADERS}
    ${LSS_HEADERS}
)
target_include_directories(lss PUBLIC ${LSS_PATH} ${VERIFMSI_PATH})

if (NOT ENABLE_LEAKSETS)
    message(STATUS "DISABLING LEAKSETS")
    # The flag is added for lss target only and as PRIVATE, this is done so that only a
    # relinking against the newly compiled lss lib will be needed, not recompiling every target
    # This is only legit because the interfaces are STRICTLY THE SAME
    target_compile_definitions(lss PRIVATE DISABLE_LEAKSETS)
endif()

file(GLOB ALEAKATOR_SOURCES ${ALEAKATOR_PATH}/*.cpp)

add_library(aleakator STATIC)
target_sources(aleakator
    PRIVATE
    ${ALEAKATOR_SOURCES}
    PUBLIC
    ${VERIF_MSI_HEADERS}
    ${CXXRTL_HEADERS}
    ${LSS_HEADERS}
    ${ALEAKATOR_HEADERS}
)
target_include_directories(aleakator
    PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${CXXRTL_PATH}
    ${VERIFMSI_PATH}
    ${LSS_PATH}
    ${ALEAKATOR_PATH}
)
