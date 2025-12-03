#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_coco-ibex.h"
#include "configuration.h"
#include "manager.h"
#include "program_setup.h"

// This file is generated
#include "programs_include.h"

bool prepare_step(Manager& manager, cxxrtl_design::p_top& top) {
    top.prev_p_clk__sys.set<bool, true>(false);
    top.p_clk__sys.set<bool, true>(true);

    return manager.step(top);
}

// Ugly trick to access memory that isn't represented as memory due to register representation
// of ram of coco's modification to ibex
// The mapping is reversed in the verilog (lowest indices mapped to last accessible data) so we
// reverse it once again here to not modify coco-ibex's core.
std::array<cxxrtl::wire<32>*, MEM_SIZE+1> RAM_POINTER_ARRAY{};
void ram(cxxrtl_design::p_top& top) {
    RAM_POINTER_ARRAY[127] = &top.p_u__ram_2e_mem_5b_0_5d_;
    RAM_POINTER_ARRAY[126] = &top.p_u__ram_2e_mem_5b_1_5d_;
    RAM_POINTER_ARRAY[125] = &top.p_u__ram_2e_mem_5b_2_5d_;
    RAM_POINTER_ARRAY[124] = &top.p_u__ram_2e_mem_5b_3_5d_;
    RAM_POINTER_ARRAY[123] = &top.p_u__ram_2e_mem_5b_4_5d_;
    RAM_POINTER_ARRAY[122] = &top.p_u__ram_2e_mem_5b_5_5d_;
    RAM_POINTER_ARRAY[121] = &top.p_u__ram_2e_mem_5b_6_5d_;
    RAM_POINTER_ARRAY[120] = &top.p_u__ram_2e_mem_5b_7_5d_;
    RAM_POINTER_ARRAY[119] = &top.p_u__ram_2e_mem_5b_8_5d_;
    RAM_POINTER_ARRAY[118] = &top.p_u__ram_2e_mem_5b_9_5d_;
    RAM_POINTER_ARRAY[117] = &top.p_u__ram_2e_mem_5b_10_5d_;
    RAM_POINTER_ARRAY[116] = &top.p_u__ram_2e_mem_5b_11_5d_;
    RAM_POINTER_ARRAY[115] = &top.p_u__ram_2e_mem_5b_12_5d_;
    RAM_POINTER_ARRAY[114] = &top.p_u__ram_2e_mem_5b_13_5d_;
    RAM_POINTER_ARRAY[113] = &top.p_u__ram_2e_mem_5b_14_5d_;
    RAM_POINTER_ARRAY[112] = &top.p_u__ram_2e_mem_5b_15_5d_;
    RAM_POINTER_ARRAY[111] = &top.p_u__ram_2e_mem_5b_16_5d_;
    RAM_POINTER_ARRAY[110] = &top.p_u__ram_2e_mem_5b_17_5d_;
    RAM_POINTER_ARRAY[109] = &top.p_u__ram_2e_mem_5b_18_5d_;
    RAM_POINTER_ARRAY[108] = &top.p_u__ram_2e_mem_5b_19_5d_;
    RAM_POINTER_ARRAY[107] = &top.p_u__ram_2e_mem_5b_20_5d_;
    RAM_POINTER_ARRAY[106] = &top.p_u__ram_2e_mem_5b_21_5d_;
    RAM_POINTER_ARRAY[105] = &top.p_u__ram_2e_mem_5b_22_5d_;
    RAM_POINTER_ARRAY[104] = &top.p_u__ram_2e_mem_5b_23_5d_;
    RAM_POINTER_ARRAY[103] = &top.p_u__ram_2e_mem_5b_24_5d_;
    RAM_POINTER_ARRAY[102] = &top.p_u__ram_2e_mem_5b_25_5d_;
    RAM_POINTER_ARRAY[101] = &top.p_u__ram_2e_mem_5b_26_5d_;
    RAM_POINTER_ARRAY[100] = &top.p_u__ram_2e_mem_5b_27_5d_;
    RAM_POINTER_ARRAY[99] = &top.p_u__ram_2e_mem_5b_28_5d_;
    RAM_POINTER_ARRAY[98] = &top.p_u__ram_2e_mem_5b_29_5d_;
    RAM_POINTER_ARRAY[97] = &top.p_u__ram_2e_mem_5b_30_5d_;
    RAM_POINTER_ARRAY[96] = &top.p_u__ram_2e_mem_5b_31_5d_;
    RAM_POINTER_ARRAY[95] = &top.p_u__ram_2e_mem_5b_32_5d_;
    RAM_POINTER_ARRAY[94] = &top.p_u__ram_2e_mem_5b_33_5d_;
    RAM_POINTER_ARRAY[93] = &top.p_u__ram_2e_mem_5b_34_5d_;
    RAM_POINTER_ARRAY[92] = &top.p_u__ram_2e_mem_5b_35_5d_;
    RAM_POINTER_ARRAY[91] = &top.p_u__ram_2e_mem_5b_36_5d_;
    RAM_POINTER_ARRAY[90] = &top.p_u__ram_2e_mem_5b_37_5d_;
    RAM_POINTER_ARRAY[89] = &top.p_u__ram_2e_mem_5b_38_5d_;
    RAM_POINTER_ARRAY[88] = &top.p_u__ram_2e_mem_5b_39_5d_;
    RAM_POINTER_ARRAY[87] = &top.p_u__ram_2e_mem_5b_40_5d_;
    RAM_POINTER_ARRAY[86] = &top.p_u__ram_2e_mem_5b_41_5d_;
    RAM_POINTER_ARRAY[85] = &top.p_u__ram_2e_mem_5b_42_5d_;
    RAM_POINTER_ARRAY[84] = &top.p_u__ram_2e_mem_5b_43_5d_;
    RAM_POINTER_ARRAY[83] = &top.p_u__ram_2e_mem_5b_44_5d_;
    RAM_POINTER_ARRAY[82] = &top.p_u__ram_2e_mem_5b_45_5d_;
    RAM_POINTER_ARRAY[81] = &top.p_u__ram_2e_mem_5b_46_5d_;
    RAM_POINTER_ARRAY[80] = &top.p_u__ram_2e_mem_5b_47_5d_;
    RAM_POINTER_ARRAY[79] = &top.p_u__ram_2e_mem_5b_48_5d_;
    RAM_POINTER_ARRAY[78] = &top.p_u__ram_2e_mem_5b_49_5d_;
    RAM_POINTER_ARRAY[77] = &top.p_u__ram_2e_mem_5b_50_5d_;
    RAM_POINTER_ARRAY[76] = &top.p_u__ram_2e_mem_5b_51_5d_;
    RAM_POINTER_ARRAY[75] = &top.p_u__ram_2e_mem_5b_52_5d_;
    RAM_POINTER_ARRAY[74] = &top.p_u__ram_2e_mem_5b_53_5d_;
    RAM_POINTER_ARRAY[73] = &top.p_u__ram_2e_mem_5b_54_5d_;
    RAM_POINTER_ARRAY[72] = &top.p_u__ram_2e_mem_5b_55_5d_;
    RAM_POINTER_ARRAY[71] = &top.p_u__ram_2e_mem_5b_56_5d_;
    RAM_POINTER_ARRAY[70] = &top.p_u__ram_2e_mem_5b_57_5d_;
    RAM_POINTER_ARRAY[69] = &top.p_u__ram_2e_mem_5b_58_5d_;
    RAM_POINTER_ARRAY[68] = &top.p_u__ram_2e_mem_5b_59_5d_;
    RAM_POINTER_ARRAY[67] = &top.p_u__ram_2e_mem_5b_60_5d_;
    RAM_POINTER_ARRAY[66] = &top.p_u__ram_2e_mem_5b_61_5d_;
    RAM_POINTER_ARRAY[65] = &top.p_u__ram_2e_mem_5b_62_5d_;
    RAM_POINTER_ARRAY[64] = &top.p_u__ram_2e_mem_5b_63_5d_;
    RAM_POINTER_ARRAY[63] = &top.p_u__ram_2e_mem_5b_64_5d_;
    RAM_POINTER_ARRAY[62] = &top.p_u__ram_2e_mem_5b_65_5d_;
    RAM_POINTER_ARRAY[61] = &top.p_u__ram_2e_mem_5b_66_5d_;
    RAM_POINTER_ARRAY[60] = &top.p_u__ram_2e_mem_5b_67_5d_;
    RAM_POINTER_ARRAY[59] = &top.p_u__ram_2e_mem_5b_68_5d_;
    RAM_POINTER_ARRAY[58] = &top.p_u__ram_2e_mem_5b_69_5d_;
    RAM_POINTER_ARRAY[57] = &top.p_u__ram_2e_mem_5b_70_5d_;
    RAM_POINTER_ARRAY[56] = &top.p_u__ram_2e_mem_5b_71_5d_;
    RAM_POINTER_ARRAY[55] = &top.p_u__ram_2e_mem_5b_72_5d_;
    RAM_POINTER_ARRAY[54] = &top.p_u__ram_2e_mem_5b_73_5d_;
    RAM_POINTER_ARRAY[53] = &top.p_u__ram_2e_mem_5b_74_5d_;
    RAM_POINTER_ARRAY[52] = &top.p_u__ram_2e_mem_5b_75_5d_;
    RAM_POINTER_ARRAY[51] = &top.p_u__ram_2e_mem_5b_76_5d_;
    RAM_POINTER_ARRAY[50] = &top.p_u__ram_2e_mem_5b_77_5d_;
    RAM_POINTER_ARRAY[49] = &top.p_u__ram_2e_mem_5b_78_5d_;
    RAM_POINTER_ARRAY[48] = &top.p_u__ram_2e_mem_5b_79_5d_;
    RAM_POINTER_ARRAY[47] = &top.p_u__ram_2e_mem_5b_80_5d_;
    RAM_POINTER_ARRAY[46] = &top.p_u__ram_2e_mem_5b_81_5d_;
    RAM_POINTER_ARRAY[45] = &top.p_u__ram_2e_mem_5b_82_5d_;
    RAM_POINTER_ARRAY[44] = &top.p_u__ram_2e_mem_5b_83_5d_;
    RAM_POINTER_ARRAY[43] = &top.p_u__ram_2e_mem_5b_84_5d_;
    RAM_POINTER_ARRAY[42] = &top.p_u__ram_2e_mem_5b_85_5d_;
    RAM_POINTER_ARRAY[41] = &top.p_u__ram_2e_mem_5b_86_5d_;
    RAM_POINTER_ARRAY[40] = &top.p_u__ram_2e_mem_5b_87_5d_;
    RAM_POINTER_ARRAY[39] = &top.p_u__ram_2e_mem_5b_88_5d_;
    RAM_POINTER_ARRAY[38] = &top.p_u__ram_2e_mem_5b_89_5d_;
    RAM_POINTER_ARRAY[37] = &top.p_u__ram_2e_mem_5b_90_5d_;
    RAM_POINTER_ARRAY[36] = &top.p_u__ram_2e_mem_5b_91_5d_;
    RAM_POINTER_ARRAY[35] = &top.p_u__ram_2e_mem_5b_92_5d_;
    RAM_POINTER_ARRAY[34] = &top.p_u__ram_2e_mem_5b_93_5d_;
    RAM_POINTER_ARRAY[33] = &top.p_u__ram_2e_mem_5b_94_5d_;
    RAM_POINTER_ARRAY[32] = &top.p_u__ram_2e_mem_5b_95_5d_;
    RAM_POINTER_ARRAY[31] = &top.p_u__ram_2e_mem_5b_96_5d_;
    RAM_POINTER_ARRAY[30] = &top.p_u__ram_2e_mem_5b_97_5d_;
    RAM_POINTER_ARRAY[29] = &top.p_u__ram_2e_mem_5b_98_5d_;
    RAM_POINTER_ARRAY[28] = &top.p_u__ram_2e_mem_5b_99_5d_;
    RAM_POINTER_ARRAY[27] = &top.p_u__ram_2e_mem_5b_100_5d_;
    RAM_POINTER_ARRAY[26] = &top.p_u__ram_2e_mem_5b_101_5d_;
    RAM_POINTER_ARRAY[25] = &top.p_u__ram_2e_mem_5b_102_5d_;
    RAM_POINTER_ARRAY[24] = &top.p_u__ram_2e_mem_5b_103_5d_;
    RAM_POINTER_ARRAY[23] = &top.p_u__ram_2e_mem_5b_104_5d_;
    RAM_POINTER_ARRAY[22] = &top.p_u__ram_2e_mem_5b_105_5d_;
    RAM_POINTER_ARRAY[21] = &top.p_u__ram_2e_mem_5b_106_5d_;
    RAM_POINTER_ARRAY[20] = &top.p_u__ram_2e_mem_5b_107_5d_;
    RAM_POINTER_ARRAY[19] = &top.p_u__ram_2e_mem_5b_108_5d_;
    RAM_POINTER_ARRAY[18] = &top.p_u__ram_2e_mem_5b_109_5d_;
    RAM_POINTER_ARRAY[17] = &top.p_u__ram_2e_mem_5b_110_5d_;
    RAM_POINTER_ARRAY[16] = &top.p_u__ram_2e_mem_5b_111_5d_;
    RAM_POINTER_ARRAY[15] = &top.p_u__ram_2e_mem_5b_112_5d_;
    RAM_POINTER_ARRAY[14] = &top.p_u__ram_2e_mem_5b_113_5d_;
    RAM_POINTER_ARRAY[13] = &top.p_u__ram_2e_mem_5b_114_5d_;
    RAM_POINTER_ARRAY[12] = &top.p_u__ram_2e_mem_5b_115_5d_;
    RAM_POINTER_ARRAY[11] = &top.p_u__ram_2e_mem_5b_116_5d_;
    RAM_POINTER_ARRAY[10] = &top.p_u__ram_2e_mem_5b_117_5d_;
    RAM_POINTER_ARRAY[9] = &top.p_u__ram_2e_mem_5b_118_5d_;
    RAM_POINTER_ARRAY[8] = &top.p_u__ram_2e_mem_5b_119_5d_;
    RAM_POINTER_ARRAY[7] = &top.p_u__ram_2e_mem_5b_120_5d_;
    RAM_POINTER_ARRAY[6] = &top.p_u__ram_2e_mem_5b_121_5d_;
    RAM_POINTER_ARRAY[5] = &top.p_u__ram_2e_mem_5b_122_5d_;
    RAM_POINTER_ARRAY[4] = &top.p_u__ram_2e_mem_5b_123_5d_;
    RAM_POINTER_ARRAY[3] = &top.p_u__ram_2e_mem_5b_124_5d_;
    RAM_POINTER_ARRAY[2] = &top.p_u__ram_2e_mem_5b_125_5d_;
    RAM_POINTER_ARRAY[1] = &top.p_u__ram_2e_mem_5b_126_5d_;
    RAM_POINTER_ARRAY[0] = &top.p_u__ram_2e_mem_5b_127_5d_;
}

template<size_t reg>
uint32_t getRegister(cxxrtl_design::p_top& top) {
	return top.p_u__core_2e_register__file__i_2e_rf__reg__tmp.curr.slice<32*reg - 1, 32*(reg-1)>().val().template get<uint32_t>();
}

int main(int argc, char *argv[]) {
    std::map<std::string, std::unique_ptr<Program>> programs;
    // This file is generated and filss the programs array
    #include "programs_instance.h"

    Configuration config(argc, argv, Configuration::CircuitType::CPU, "coco-ibex");
    cxxrtl_design::p_top top;
    config.EXCEPTIONS_WORD_VERIF_["u_core register_file_i rf_reg_tmp"] = 32;
    Manager manager(top, config);

    // There are assertions before about non existing programs but
    // formally perform it here where the list is definitive
    if (not programs.contains(manager.config_.subprogram_))
        throw std::invalid_argument( "Program does not exist in driver memory." );
    std::unique_ptr<Program>& program = programs[manager.config_.subprogram_];

    ram(top);

    // Call the loader and symbol initializer of the program
    program->load(top);
    assert(program->symbols_.contains("_start") && "_start symbol is mandatory for cpus.");
    assert(program->symbols_.contains("_hang") && "_hang symbol is mandatory for cpus.");

    // Reset
    top.p_rst__sys__n.set<bool, true>(false);
    top.p_rst__sys__n.set_fully_stable();
    prepare_step(manager, top);
    prepare_step(manager, top);
    top.p_rst__sys__n.set<bool, true>(true);
    top.p_rst__sys__n.set_fully_stable();

    // Initialisation
    bool init_done = false;
    for (int i = 0; i < 5000; ++i) {
        prepare_step(manager, top);
        std::cout << "Init cycle : " << i << " --------------------------" << std::endl;
        std::cout << "PC: " << std::hex << program->pc(top) << std::dec << std::endl;
        std::cout << "SP : " << std::hex << getRegister<2>(top) << std::dec << std::endl;

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
    for (int i = 0; i < 50000; ++i) {
        prepare_step(manager, top);

        if (top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_31_40_.curr.concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_30_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_29_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_28_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_27_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_26_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_25_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_24_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_23_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_22_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_21_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_20_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_19_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_18_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_17_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_16_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_15_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_14_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_13_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_12_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_11_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_10_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_9_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_8_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_7_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_6_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_5_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_4_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_3_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_2_40_.curr)
                .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_1_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_0_40_).val().node->nature != CONST ||
            top.p_u__core_2e_id__stage__i_2e_controller__i_2e_instr__i.curr.node->nature != CONST) {
            std::cout << "PC or fetched instr was symbolized." << std::endl;
            raise(SIGTRAP);
        }


        std::cout << "--------------------------" << std::endl;
        std::cout << "Values following will be used as 'curr' for next cycle:" << std::endl;
//        std::cout << "Registers : " << *top.p_u__core_2e_register__file__i_2e_rf__reg__tmp.curr.node << std::endl;
        std::cout << "SP : " << std::hex << getRegister<2>(top) << std::dec << std::endl;
        std::cout << "PC: " << std::hex << program->pc(top) << std::dec << std::endl;
//        std::cout << "Reg: " << *top.p_u__core_2e_register__file__i_2e_rf__reg__tmp.curr.node << std::endl;

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
