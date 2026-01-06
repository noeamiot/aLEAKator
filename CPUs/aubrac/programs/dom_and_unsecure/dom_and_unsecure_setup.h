#ifndef DOM_AND_UNSECURE_SETUP_H
#define DOM_AND_UNSECURE_SETUP_H

#include "aubrac_program.h"

class dom_and_unsecure : public AubracProgram<dom_and_unsecure> {
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
        Node* m;
};
#endif
