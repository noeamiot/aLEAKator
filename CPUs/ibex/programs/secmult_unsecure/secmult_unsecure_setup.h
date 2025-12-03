#ifndef SECMULT_UNSECURE_SETUP_H
#define SECMULT_UNSECURE_SETUP_H
#include "ibex_program.h"

class secmult_unsecure : public IbexProgram<secmult_unsecure> {
    public:
        void init_implem(Manager& manager, cxxrtl_design::p_top& top);
        void conclude_implem(Manager& manager, cxxrtl_design::p_top& top);
        void hook_implem(Manager& manager, cxxrtl_design::p_top& top);
        void load_implem(cxxrtl_design::p_top& top);
        void symbols_implem();

    private:
        // Program specifics
        Node* a;
        Node* b;
};
#endif
