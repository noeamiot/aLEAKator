#include <iostream>
#include "configuration.h"
#include "manager.h"
#include "program_setup.h"
#include "aubrac_include.h"

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

    Configuration config(argc, argv, Configuration::CircuitType::CPU, CORE_VERSION);
    config.EXCEPTIONS_WORD_VERIF_["u_top m_pipe m_back m_gpr _GEN"] = 32;
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

    // Reset (5 stage pipeline so at least 6 to be sure)
    top.p_IO__RST__N.set<bool, true>(true);
    prepare_step(manager, top);
    prepare_step(manager, top);
    prepare_step(manager, top);
    prepare_step(manager, top);
    prepare_step(manager, top);
    prepare_step(manager, top);
    top.p_IO__RST__N.set<bool, true>(false);

    // Initialisation
    bool init_done = false;
    for (int i = 0; i < 5000; ++i) {
        prepare_step(manager, top);
        std::cout << "Init cycle : " << i << " --------------------------" << std::endl;
        std::cout << "PC: " << std::hex << program->pc(top) << std::dec << std::endl;
        std::cout << "Read port 0: " << std::hex << top.p_u__top__ram_2e___m__intf__io__b__read__1__data.next << std::endl;
        std::cout << "Read port 1: " << std::hex << top.p_u__top__ram_2e___m__ram__io__b__port__0__rdata.next << std::endl;

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

        if (top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_31_40_.curr.concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_30_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_29_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_28_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_27_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_26_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_25_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_24_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_23_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_22_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_21_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_20_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_19_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_18_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_17_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_16_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_15_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_14_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_13_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_12_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_11_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_10_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_9_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_8_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_7_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_6_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_5_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_4_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_3_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_2_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_1_40_).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_0_40_).val().node->nature != CONST) {
            std::cout << "PC is symbolized" << std::endl;
            raise(SIGTRAP);
        }


        std::cout << "--------------------------" << std::endl;
//        std::cout << "instr: " << *top.p_u__top_2e_u__ibex__core_2e_id__stage__i_2e_controller__i_2e_instr__i.curr.node << std::endl;
//        std::cout << "stab:" << std::hex << top.p_u__top_2e_u__ibex__core_2e_id__stage__i_2e_controller__i_2e_instr__i.curr.stability[0] << std::endl;
        std::cout << "PC: " << std::hex << program->pc(top) << std::dec << std::endl;
        std::cout << "PC: " << top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_31_40_.curr.concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_30_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_29_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_28_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_27_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_26_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_25_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_24_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_23_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_22_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_21_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_20_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_19_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_18_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_17_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_16_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_15_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_14_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_13_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_12_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_11_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_10_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_9_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_8_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_7_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_6_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_5_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_4_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_3_40_.curr).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_2_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_1_40_).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_0_40_).val().node->verbatimPrint() << std::endl;
        std::cout << "PC_next: " << top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_31_40_.curr.concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_30_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_29_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_28_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_27_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_26_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_25_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_24_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_23_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_22_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_21_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_20_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_19_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_18_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_17_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_16_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_15_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_14_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_13_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_12_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_11_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_10_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_9_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_8_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_7_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_6_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_5_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_4_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_3_40_.curr).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_2_40_.curr)
            .concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_1_40_).concat(top.p_u__top_2e_m__pipe_2e_m__front_2e_m__pc_2e_r__pc__next_40_0_40_).val().node->verbatimPrint() << std::endl;

        // Call hook for the program
        program->hook(manager, top);

        // Please keep in mind that the hand symbol is here only for the provided link and startup scripts
        if (program->symbols_["_hang"].addr == program->pc(top)) {
            std::cout << "Reached hang symbol, stopping aubrac." << std::endl;
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
