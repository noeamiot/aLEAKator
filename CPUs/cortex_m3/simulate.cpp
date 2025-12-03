#include <cstdint>
#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_cortex_m3.h"
#include "configuration.h"
#include "manager.h"

// This file is generated
#include "programs_include.h"

bool prepare_step(Manager& manager, cxxrtl_design::p_top& top) {
    top.prev_p_HCLK.set<bool, true>(false);
    top.p_HCLK.set<bool, true>(true);

    return manager.step(top);
}

int main(int argc, char *argv[]) {
    std::map<std::string, std::unique_ptr<Program>> programs;
    // This file is generated and fills the programs array
    #include "programs_instance.h"

    Configuration config(argc, argv, Configuration::CircuitType::CPU, "cortex_m3");
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
    top.p_HRESETn.set<bool, true>(false);
    prepare_step(manager, top);
    prepare_step(manager, top);
    top.p_HRESETn.set<bool, true>(true);

    // Unwind until start of program
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
    uint32_t instr_in_exec = 0x00000000u;
    uint32_t pc_exec = 0x00000000u;
    // Tentative for a matching PC with hardware
    uint32_t pc_correct_try = 0x00000000u;

    for (int i = 0; i < 150000; ++i) {
        // Permanent concretization of the 1k signal
        if (top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__lsu_2e_u__cm3__dpu__lsu__ahbintf_2e_addrd__1k__cross.curr.node->nature != CONST) {
            std::cout << "Concretized 1k bug" << std::endl;
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__lsu_2e_u__cm3__dpu__lsu__ahbintf_2e_addrd__1k__cross.curr.setNode(&constant(top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__lsu_2e_u__cm3__dpu__lsu__ahbintf_2e_addrd__1k__cross.curr.get<uint8_t>(), 1));
        }
        if (top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__regbank_2e_u__cm3__dpu__regfile_2e_reg13.curr.node->nature != CONST
            || top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__regbank_2e_rf__pc__ex.node->nature != CONST
            || top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_instr__de.curr.node->nature != CONST
        ) {
            std::cout << "PC, SP or fetch instruction is symbolized" << std::endl;
            raise(SIGTRAP);
        }

        // Display registers curr (t ff output value)
        std::cout << "--------------------------" << std::endl;
        std::cout << "Following values are the output values of ff's at cycle: " << manager.get_steps() << std::endl;
        uint32_t decode_instr_reg = top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_instr__de.curr.get<uint32_t>();
        std::cout << "Decode instr(R): " << std::hex << decode_instr_reg << std::dec << std::endl;
        std::cout << "instr_advance_ex(R): " << *top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_instr__advance__ex.curr.node << std::endl;
        std::cout << "SP: " << *top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__regbank_2e_u__cm3__dpu__regfile_2e_reg13.curr.node << std::endl;
        uint32_t pc_forward = 0 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_1_40_.curr.get<uint32_t>() << 1 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_2_40_.curr.get<uint32_t>() << 2 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_3_40_.curr.get<uint32_t>() << 3 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_4_40_.curr.get<uint32_t>() << 4 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_5_40_.curr.get<uint32_t>() << 5 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_6_40_.curr.get<uint32_t>() << 6 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_7_40_.curr.get<uint32_t>() << 7 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_8_40_.curr.get<uint32_t>() << 8 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_9_40_.curr.get<uint32_t>() << 9 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_10_40_.curr.get<uint32_t>() << 10 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_11_40_.curr.get<uint32_t>() << 11 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_12_40_.curr.get<uint32_t>() << 12 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_13_40_.curr.get<uint32_t>() << 13 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_14_40_.curr.get<uint32_t>() << 14 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_15_40_.curr.get<uint32_t>() << 15 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_16_40_.curr.get<uint32_t>() << 16 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_17_40_.curr.get<uint32_t>() << 17 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_18_40_.curr.get<uint32_t>() << 18 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_19_40_.curr.get<uint32_t>() << 19 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_20_40_.curr.get<uint32_t>() << 20 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_21_40_.curr.get<uint32_t>() << 21 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_22_40_.curr.get<uint32_t>() << 22 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_23_40_.curr.get<uint32_t>() << 23 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_24_40_.curr.get<uint32_t>() << 24 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_25_40_.curr.get<uint32_t>() << 25 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_26_40_.curr.get<uint32_t>() << 26 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_27_40_.curr.get<uint32_t>() << 27 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_28_40_.curr.get<uint32_t>() << 28 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_29_40_.curr.get<uint32_t>() << 29 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_30_40_.curr.get<uint32_t>() << 30 |
            top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__pc__fwd__ex_40_31_40_.curr.get<uint32_t>() << 31;
        std::cout << "PC(R) " << std::hex << pc_forward << std::dec << std::endl;
        std::cout << "instr_align_ex: " << std::hex << *top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_instr__algn__ex.curr.node << std::dec << std::endl;
        //std::cout << "status_flags: " << *top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__status_2e_flags__ex.curr.node << std::endl;
        //std::cout << "read port a ex: " << std::hex << *top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__rd__a__data__ex.curr.node << std::dec << std::endl;
        //std::cout << "read port b ex: " << std::hex << *top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_rf__rd__b__data__ex.curr.node << std::dec << std::endl;

        // Compute comb and advance register curr
        prepare_step(manager, top);

        // Display comb values (they are resulting of the computed cycle, and correspond to the cycle t)
        std::cout << "Following values are the combinatorial values of cycle: " << manager.get_steps()-1 << std::endl;
        uint32_t decoded_instr = (decode_instr_reg & (top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__dec_2e_instr__size__de.get<uint32_t>() ? 0xFFFFFFFFu : 0x0000FFFFu ));
        std::cout << "Decoded instruction used: " << std::hex << decoded_instr << std::dec << std::endl;
        std::cout << "Executed instruction: " << std::hex << instr_in_exec << std::dec << std::endl;

        std::cout << "PC_decode: " << std::hex << pc_forward-4 << std::dec << std::endl;
        std::cout << "PC_exec: " << std::hex << pc_exec << std::dec << std::endl;
        std::cout << "PC_corect_try: " << std::hex << pc_correct_try << std::dec << std::endl;

        std::cout << "lss_pipe_adv_de: " << std::hex << top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__lsu_2e_u__cm3__dpu__lsu__ctl_2e_lss__pipe__adv__de.get<uint32_t>() << std::dec << std::endl;

        if (top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_instr__de__2__ex__en.get<uint32_t>()) {
            instr_in_exec = decoded_instr;
            pc_exec = pc_forward-4;
        }
        if (not top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__lsu_2e_u__cm3__dpu__lsu__ctl_2e_lss__pipe__adv__de.get<uint32_t>()) {
            std::cout << "No pipelining in LSU" << std::endl;
            pc_correct_try = pc_exec;
        }
        // End display

        // Call hook for the program
        program->hook(manager, top);

        // Please keep in mind that the hand symbol is here only for the provided link and startup scripts
        if (program->symbols_["_hang"].addr == program->pc(top)) {
            std::cout << "Reached hang symbol, stopping cortex_m3." << std::endl;
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
