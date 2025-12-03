#ifndef PROGRAM_CM3_SETUP_H
#define PROGRAM_CM3_SETUP_H

#include "program_setup.h"
#include "cxxrtl_cortex_m3.h"

template<class Derived>
class CM3Program : public ProgramInterface<Derived> {
    public:
        static const inline std::map<std::string, unsigned int> memory_mapping_ = {
            {"ram", 0x20000000u},
            {"rom", 0x0u}
        };

        uint32_t pc_implem(cxxrtl_design::p_top& top) {
            return top.p_uCORTEXM3INTEGRATION_2e_uCORTEXM3_2e_u__cm3__dpu_2e_u__cm3__dpu__regbank_2e_rf__pc__ex.get<uint32_t>();
        }
};

#endif // PROGRAM_CM3_SETUP_H
