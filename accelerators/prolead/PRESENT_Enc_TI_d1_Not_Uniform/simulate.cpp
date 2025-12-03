#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_prolead-present_enc_ti_d1_not_uniform.h"
#include "manager.h"

bool prepare_step(Manager& manager, cxxrtl_design::p_top& top) {
    // WARNING: ALWAYS SIMULATE THAT WE HAVE DONE A NEGEDGE BEFORE
    top.prev_p_clk.set<bool, true>(false);
    top.p_clk.set<bool, true>(true);

    return manager.step(top);
}

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::ACCELERATOR, "prolead-present_enc_ti_d1_not_uniform");
    config.REMOVE_FALSE_NEGATIVE_ = true;
    cxxrtl_design::p_top top;
    Manager manager(top, config);

    uint64_t X = 0xFFFFFFFFFFFFFFFF;

    uint64_t X0, X1, X2;
    X0 = rand();
    X1 = rand();
    X2 = X^X0^X1;

    top.p_reset.set<bool, true>(true);
    prepare_step(manager, top);
    manager.begin_measure();
    top.p_reset.set<bool, true>(false);
    prepare_step(manager, top);

    #ifndef CONST_EXEC
        top.p_data__in1.set<uint64_t>(X0);
        top.p_data__in2.set<uint64_t>(X1);
        top.p_data__in3.set<uint64_t>(X2);
        top.p_key = cxxrtl::value<80>{0xffffffffu,0xffffffffu,0xffffu};

        Node& pt = symbol("pt", 'S', 64);
        std::vector<Node*> spt = manager.get_shares(pt, 3);
        top.p_data__in1.setNode(spt[0]);
        top.p_data__in2.setNode(spt[1]);
        top.p_data__in3.setNode(spt[2]);
    #else
        top.p_data__in1.set<uint64_t, true>(X0);
        top.p_data__in2.set<uint64_t, true>(X1);
        top.p_data__in3.set<uint64_t, true>(X2);
        top.p_key = cxxrtl::value<80>{0xffffffffu,0xffffffffu,0xffffu};
    #endif
    prepare_step(manager, top);
    prepare_step(manager, top);

    while (top.p_done.get<bool>() != true) {
        prepare_step(manager, top);
    }

    manager.end_measure();
    manager.stat();

    uint64_t Q0, Q1, Q2, Q0b, Q1b, Q2b;
    Q0 = top.p_data__out1.get<uint64_t>();
    Q1 = top.p_data__out2.get<uint64_t>();
    Q2 = top.p_data__out3.get<uint64_t>();
    Q0b = top.p_data__out1.getNode()->cst[0];
    Q1b = top.p_data__out2.getNode()->cst[0];
    Q2b = top.p_data__out3.getNode()->cst[0];

    std::cout << "End of computation" << std::endl;
    uint64_t Q = Q0 ^ Q1 ^Q2;
    [[maybe_unused]] uint64_t Qb = Q0b ^ Q1b ^Q2b;

    if (Q != 0x3333DCD3213210D2u)
        printf("Error. \n\n");
    else
        printf("OK. \n\n");

    assert(Q == Qb);
    assert(Q == 0x3333DCD3213210D2u);
}

