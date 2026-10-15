#ifndef SECMULT_SETUP_H
#define SECMULT_SETUP_H

#include "ibex_program.h"

class secmult : public IbexProgram<secmult> {
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
