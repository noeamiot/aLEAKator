#ifndef SECMULT_UNSECURE_SETUP_H
#define SECMULT_UNSECURE_SETUP_H

#include "cortex_m4_program.h"

class secmult_unsecure : public CM4Program<secmult_unsecure> {
    public:
        void initial_init_implem(Manager& manager);
        void init_implem(Manager& manager, cxxrtl_design::p_top& top);
        void conclude_implem(Manager& manager, cxxrtl_design::p_top& top);
        void hook_implem(Manager& manager, cxxrtl_design::p_top& top);

    private:
        // Program specifics
        Node* a;
        Node* b;
        Node* m;

        std::vector<Node*> sa;
        std::vector<Node*> sb;
};
#endif
