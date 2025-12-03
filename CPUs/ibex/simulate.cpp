#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_ibex.h"
#include "configuration.h"
#include "manager.h"
#include "program_setup.h"

// This file is generated
#include "programs_include.h"

bool prepare_step(Manager& manager, cxxrtl_design::p_top& top) {
    top.prev_p_IO__CLK.set<bool, true>(false);
    top.p_IO__CLK.set<bool, true>(true);

    return manager.step(top);
}

int main(int argc, char *argv[]) {
    std::map<std::string, std::unique_ptr<Program>> programs;
    // This file is generated and filss the programs array
    #include "programs_instance.h"

    Configuration config(argc, argv, Configuration::CircuitType::CPU, "ibex");
    cxxrtl_design::p_top top;
    Manager manager(top, config);

    // There are assertions before about non existing programs but
    // formally perform it here where the list is definitive
    if (not programs.contains(manager.config_.subprogram_))
        throw std::invalid_argument( "Program does not exist in driver memory." );
    std::unique_ptr<Program>& program = programs[manager.config_.subprogram_];

    // Call the loader and symbol initializer of the program
    program->load(top);
    assert(program->symbols_.contains("_start") && "_start symbol is mandatory for cpus.");
    assert(program->symbols_.contains("_hang") && "_hang symbol is mandatory for cpus.");

    // Reset
    top.p_IO__RST__N.set<bool, true>(false);
    prepare_step(manager, top);
    prepare_step(manager, top);
    top.p_IO__RST__N.set<bool, true>(true);

    // Initialisation
    bool init_done = false;
    for (int i = 0; i < 5000; ++i) {
        prepare_step(manager, top);
        std::cout << "Init cycle : " << i << " --------------------------" << std::endl;
        std::cout << "PC: " << std::hex << program->pc(top) << std::dec << std::endl;

        if (program->symbols_["_start"].addr == program->pc(top)) {
            std::cout << "Completed initialization." << std::endl;
            init_done = true;
            break;
        }

        if (program->symbols_["_hang"].addr == program->pc(top)) {
            std::cout << "Reached hang symbol before end of init, this is an issue." << std::endl;
            std::exit(EXIT_SUCCESS);
        }
    }

    // Either not enough init cycles or a fault
    if (!init_done) {
        std::cout << "Unwinding of cycles did not lead to crossing the _start symbol, stopping." << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    // Call the initializer of the program
    program->init(manager, top);

    bool reached_end = false;
    for (int i = 0; i < 25000; ++i) {
        prepare_step(manager, top);

        if (top.p_u__top_2e_u__ibex__core_2e_id__stage__i_2e_controller__i_2e_instr__i.curr.node->nature != CONST
            || top.p_u__top_2e_u__ibex__core_2e_cs__registers__i_2e_pc__id__i.node->nature != CONST
        ) {
            std::cout << "PC or fetch instruction is symbolized" << std::endl;
            raise(SIGTRAP);
        }


        std::cout << "--------------------------" << std::endl;
        std::cout << "instr: " << *top.p_u__top_2e_u__ibex__core_2e_id__stage__i_2e_controller__i_2e_instr__i.curr.node << std::endl;
        std::cout << "stab:" << std::hex << top.p_u__top_2e_u__ibex__core_2e_id__stage__i_2e_controller__i_2e_instr__i.curr.stability[0] << std::endl;
        std::cout << "PC: " << std::hex << program->pc(top) << std::dec << std::endl;

        // Call hook for the program
        program->hook(manager, top);

        // Please keep in mind that the hand symbol is here only for the provided link and startup scripts
        if (program->symbols_["_hang"].addr == program->pc(top)) {
            std::cout << "Reached hang symbol, stopping ibex." << std::endl;
            reached_end = true;
            break;
        }
    }

    if (!reached_end) {
        std::cout << "Stopped before reaching end symbol, this is a big issue." << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    // Call the output handler of the program
    program->conclude(manager, top);
    manager.stat();
    std::exit(EXIT_SUCCESS);
}
