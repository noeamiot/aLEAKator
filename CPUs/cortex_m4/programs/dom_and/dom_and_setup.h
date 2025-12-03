#ifndef DOM_AND_SETUP_H
#define DOM_AND_SETUP_H

#include "cortex_m4_program.h"

class dom_and : public CM4Program<dom_and> {
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
