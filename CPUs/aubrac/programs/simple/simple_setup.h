#ifndef SIMPLE_SETUP_H
#define SIMPLE_SETUP_H

#include "aubrac_program.h"

class simple : public AubracProgram<simple> {
    public:
        void initial_init_implem(Manager& manager);
        void init_implem(Manager& manager, cxxrtl_design::p_top& top);
        void conclude_implem(Manager& manager, cxxrtl_design::p_top& top);
        void hook_implem(Manager& manager, cxxrtl_design::p_top& top);

    private:
        // Program specifics
        Node* a;

        std::vector<Node*> sa;
};
#endif
