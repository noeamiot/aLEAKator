#ifndef PROGRAM_IBEX_SETUP_H
#define PROGRAM_IBEX_SETUP_H

#include "program_setup.h"
#include "cxxrtl_ibex.h"

template<class Derived>
class IbexProgram : public ProgramInterface<Derived> {
    public:
        static const inline std::map<std::string, unsigned int> memory_mapping_ = {
            {"ram", 0x100000u},
        };

        // Matching LD script, text and data section are concatenated at beginning of ram
        void load_implem(Manager& manager, cxxrtl_design::p_top& top) {
            // Ask manager to (re)program and perform (TODO:) a microarchitectural reset
            this->program_ = manager.load_program();

            size_t index = this->get_index("ram", "_text_start");
            for (const auto& word : this->program_.text_) {
                top.memory_p_u__ram_2e_u__ram_2e_mem[index].set<uint32_t, true>(word);
                index++;
            }

            index = this->get_index("ram", "_data_start");
            for (const auto& word : this->program_.data_) {
                top.memory_p_u__ram_2e_u__ram_2e_mem[index].set<uint32_t, true>(word);
                index++;
            }
        }

        uint32_t pc_implem(cxxrtl_design::p_top& top) {
            return top.p_u__top_2e_u__ibex__core_2e_cs__registers__i_2e_pc__id__i.get<uint32_t>();
        }
};

#endif // PROGRAM_IBEX_SETUP_H
