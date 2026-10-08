#ifndef PROGRAM_AUBRAC_SETUP_H
#define PROGRAM_AUBRAC_SETUP_H

#include "aubrac_include.h"
#include "program_setup.h"

template<class Derived>
class AubracProgram : public ProgramInterface<Derived> {
    public:
        static const inline std::map<std::string, unsigned int> memory_mapping_ = {
            {"ram", 0x04000000u},
        };

        // Matching LD script, text and data section are concatenated at beginning of ram
        void load_implem(Manager& manager, cxxrtl_design::p_top& top) {
            this->program_ = manager.load_program();

            size_t index = this->get_index("ram", "_text_start");
            for (const auto& word : this->program_.text_) {
                top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[index].set<uint32_t, true>(word);
                index++;
            }

            index = this->get_index("ram", "_data_start");
            for (const auto& word : this->program_.data_) {
                top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[index].set<uint32_t, true>(word);
                index++;
            }
        }

        uint32_t pc_implem(cxxrtl_design::p_top& top) {
            return top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_31_40_.curr.concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_30_40_.curr)
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
            .concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_1_40_).concat(top.p_u__top_2e_m__pipe_2e___m__front__io__b__out__0__ctrl__pc_40_0_40_).val().get<uint32_t>();
        }
};

#endif // PROGRAM_AUBRAC_SETUP_H
