#ifndef PROGRAM_CM4_SETUP_H
#define PROGRAM_CM4_SETUP_H

#include "program_setup.h"
#include "cxxrtl_cortex_m4.h"

template<class Derived>
class CM4Program : public ProgramInterface<Derived> {
    public:
        static const inline std::map<std::string, unsigned int> memory_mapping_ = {
            {"ram", 0x20000000u},
            {"rom", 0x0u}
        };

        uint32_t pc_implem(cxxrtl_design::p_top& top) {
            return top.p_uCORTEXM4INTEGRATION_2e_uCORTEXM4_2e_u__cm4__dpu_2e_u__cm4__dpu__regbank_2e_rf__pc__ex.get<uint32_t>();
        }
};

#endif // PROGRAM_CM4_SETUP_H
