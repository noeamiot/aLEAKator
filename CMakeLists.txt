cmake_minimum_required(VERSION 3.28)

# User can specify another LLVM+CLANG instalation than the default one
set(LLVM_ROOT "" CACHE PATH "")
option(LLVM_ROOT "A specific LLVM/Clang installation to use" "")

if (LLVM_ROOT)
    set(CMAKE_C_COMPILER "${LLVM_ROOT}/bin/clang" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "${LLVM_ROOT}/bin/clang++" CACHE FILEPATH "C++ compiler" FORCE)
    set(CMAKE_LINKER ${LLVM_ROOT}/bin/ld.lld)
else()
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_CXX_COMPILER "clang++")
    set(CMAKE_LINKER "ld.lld")
endif()

project(aleakator_all LANGUAGES C CXX)

# Force the compiler to be clang, this is a requirement for cxxrtl model building
if (NOT CMAKE_C_COMPILER_ID STREQUAL "Clang"
    OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR
        "aLEAKator requires Clang.\n"
        "C compiler: ${CMAKE_C_COMPILER}\n"
        "C++ compiler: ${CMAKE_CXX_COMPILER}"
    )
endif()

if(STATIC_BUILD)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    set(Boost_USE_STATIC_LIBS ON)
    set(ZLIB_USE_STATIC_LIBS ON)
    add_link_options(-static)
    message(STATUS "Will compile statically")
endif()

# Default value for build type, will be cached
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Default value for lss disable, will NOT be cached
if(NOT DEFINED ENABLE_LEAKSETS)
    set(ENABLE_LEAKSETS ON)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(NOT DEFINED VERIFMSI_PATH)
    set(VERIFMSI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../verif_msi_pp/")
endif()
set(VERIFMSI_HEADERS ${VERIFMSI_PATH}/include/)

set(CXXRTL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/")
set(CXXRTL_HEADER ${CXXRTL_PATH}/cxxrtl/cxxrtl.h ${CXXRTL_PATH}/cxxrtl/capi/cxxrtl_capi.h)

set(LSS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/lss/")
set(LSS_HEADERS ${LSS_PATH}/lss.h)

set(ALEAKATOR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/aleakator/")
file(GLOB ALEAKATOR_HEADERS ${ALEAKATOR_PATH}/*.h)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)

if(LLVM_ROOT)
    find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT})
    find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT})
else()
    find_package(LLVM REQUIRED CONFIG)
    find_package(Clang REQUIRED CONFIG)
endif()

include(AddLLVM)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

# Be careful, these options are defined for ALL compiled elements, from lib to C programs of CPUs
# The warnings excluded from -Werror are due to false positives in clang, see https://github.com/llvm/llvm-project/issues/61025
add_compile_options(-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-error=shift-count-overflow -Wno-error=array-bounds)
add_definitions(-DMUST_BIT_DECOMPOSE)

# Additionnal compile option for state debugging
option(DEBUG_STATE "Internal model state consistency checks" OFF)
if(DEBUG_STATE)
    message(STATUS "DEBUG_STATE is defined, models will be extremely slow")
    add_definitions(-DDEBUG_STATE_CONSISTANCE)
endif()

# Check that default or found yosys is ok for use in aleakator
if(NOT DEFINED YOSYS_CMD)
    find_program(YOSYS_CMD yosys REQUIRED)
endif()
execute_process(
    COMMAND ${YOSYS_CMD} -p "write_cxxrtl -print-symbolic"
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
    RESULT_VARIABLE YOSYS_CXXRTL_SYMB_SUPPORT
)
if(NOT YOSYS_CXXRTL_SYMB_SUPPORT STREQUAL "0")
    message(FATAL_ERROR "Wrong yosys version, does not support 'write_cxxrtl -print-symbolic': ${YOSYS_CXXRTL_SYMB_SUPPORT}")
endif()
message(STATUS "Yosys with CXXRTL Symbolic support: Found ${YOSYS_CMD}")

find_program(SV2V_CMD sv2v REQUIRED)
message(STATUS "Sv2v: Found ${SV2V_CMD}")

if(NOT DEFINED GHDL_ENABLE)
    set(GHDL_ENABLE ON)
endif()

if (GHDL_ENABLE)
    set(GHDL_MODULE "ghdl")
    execute_process(
        COMMAND ${YOSYS_CMD} -m ${GHDL_MODULE} -p "check"
        OUTPUT_QUIET
        RESULT_VARIABLE YOSYS_GHDL_SUPPORT
    )
    if(NOT YOSYS_GHDL_SUPPORT STREQUAL "0")
        message(FATAL_ERROR "GHDL support was requested but yosys fails to load the module: ${YOSYS_GHDL_SUPPORT}")
    endif()
    message(STATUS "Yosys supports GHDL module loading: OK")
endif()

set(YOSYS_OPTS "-g" "--logfile" "synth.log" "--quiet")
set(YOSYS_COMMON_STEPS "flatten; proc; pmuxtree; opt_expr; opt_clean;;;  check; opt -nodffe -nosdff; fsm; opt; peepopt; opt_clean; share; opt; memory -nomap; opt_clean; opt -fast -full; memory_map; opt -full; opt_clean; dffunmap; rename -enumerate;")
set(CXXRTL_OPTS "-O6 -g4 -header -print-symbolic")

find_package(Boost 1.83 CONFIG REQUIRED COMPONENTS program_options)


# Define the verif_msi_pp library specifics
add_library(verif_msi_pp STATIC IMPORTED)
set(VERIFMSI_LIBRARY "${VERIFMSI_PATH}/libverif_msi_pp.a")
if(NOT EXISTS "${VERIFMSI_LIBRARY}")
    message(FATAL_ERROR "verif_msi_pp library not found: ${VERIFMSI_LIBRARY}")
endif()
set_target_properties(verif_msi_pp PROPERTIES
  IMPORTED_LOCATION "${VERIFMSI_LIBRARY}"
)

add_library(cxxrtl INTERFACE ${CXXRTL_HEADERS})
target_include_directories(cxxrtl INTERFACE ${CXXRTL_PATH})

enable_testing()

# Build aleakator and lss libs
add_subdirectory(libs)

# Build CPUs and their programs
add_subdirectory(CPUs)

# Build other hardware accelerators
add_subdirectory(accelerators)

# Build unit tests
add_subdirectory(utests)
