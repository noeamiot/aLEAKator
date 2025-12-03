#ifndef PROGRAM_IBEX_SETUP_H
#define PROGRAM_IBEX_SETUP_H

#include "program_setup.h"
#include "cxxrtl_coco-ibex.h"

#define MEM_SIZE 127
extern std::array<cxxrtl::wire<32>*, MEM_SIZE+1> RAM_POINTER_ARRAY;

template<class Derived>
class CocoIbexProgram : public ProgramInterface<Derived> {
    public:
        // Not a bug, coco-ibex has overlapping memory regions mapped to different memories ...
        static const inline std::map<std::string, unsigned int> memory_mapping_ = {
            {"ram", 0x0u},
            {"rom", 0x0u}
        };

        uint32_t pc_implem(cxxrtl_design::p_top& top) {
            return top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_31_40_.curr.concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_30_40_.curr)
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
            .concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_1_40_.curr).concat(top.p_u__core_2e_cs__registers__i_2e_pc__id__i_40_0_40_).val().get<uint32_t>();
        }
        
        // Coco-ibex is handled differently
        std::array<cxxrtl::wire<32>*, MEM_SIZE+1>& ram = RAM_POINTER_ARRAY;
};

#endif // PROGRAM_IBEX_SETUP_H
