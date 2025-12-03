#ifndef PROGRAM_CV32E40P_SETUP_H
#define PROGRAM_CV32E40P_SETUP_H

#include "program_setup.h"
#include "cxxrtl_cv32e40p.h"

template<class Derived>
class Cv32e40pProgram : public ProgramInterface<Derived> {
    public:
        static const inline std::map<std::string, unsigned int> memory_mapping_ = {
            {"ram", 0x100000u},
            {"rom", 0x0u}
        };

        uint32_t pc_implem(cxxrtl_design::p_top& top) {
            return top.p_u__top_2e_core__i_2e_cs__registers__i_2e_pc__id__i.curr.get<uint32_t>();
        }
};

#endif // PROGRAM_CV32E40P_SETUP_H
