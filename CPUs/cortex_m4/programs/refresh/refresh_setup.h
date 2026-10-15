#ifndef REFRESH_SETUP_H
#define REFRESH_SETUP_H

#include "cortex_m4_program.h"

class refresh : public CM4Program<refresh> {
    public:
        void initial_init_implem(Manager& manager);
        void init_implem(Manager& manager, cxxrtl_design::p_top& top);
        void conclude_implem(Manager& manager, cxxrtl_design::p_top& top);
        void hook_implem(Manager& manager, cxxrtl_design::p_top& top);

    private:
        // Program specifics
        Node* a;
        Node* m;

        std::vector<Node*> sa;
};
#endif
