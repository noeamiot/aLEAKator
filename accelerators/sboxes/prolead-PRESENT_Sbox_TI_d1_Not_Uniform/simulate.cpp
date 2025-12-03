#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_prolead-present_sbox_ti_d1_not_uniform.h"
#include "manager.h"

int sbox[16] = { 0xC, 0x5, 0x6, 0xB, 0x9, 0x0, 0xA, 0xD, 0x3, 0xE, 0xF, 0x8, 0x4, 0x7, 0x1, 0x2 };

bool prepare_step(Manager& manager, cxxrtl_design::p_top& top) {
    // WARNING: ALWAYS SIMULATE THAT WE HAVE DONE A NEGEDGE BEFORE
    top.prev_p_clk.set<bool, true>(false);
    top.p_clk.set<bool, true>(true);

    return manager.step(top);
}

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::ACCELERATOR, "prolead-present_sbox_ti_d1_not_uniform");
    config.REMOVE_FALSE_NEGATIVE_ = true;
    cxxrtl_design::p_top top;
    Manager manager(top, config);

    int X, Y;
    X = rand() & 0xF;
    X=X+1;
    Y = sbox[X];

    int X0, X1, X2;
    X0 = rand()  & 0xF;
    X1 = rand()  & 0xF;
    X2 = X^X0^X1;

    #ifndef CONST_EXEC
        top.p_sboxIn1.set<uint8_t>(X0);
        top.p_sboxIn2.set<uint8_t>(X1);
        top.p_sboxIn3.set<uint8_t>(X2);

        Node& sec = symbol("sec", 'S', 4);
        std::vector<Node*> ssec = manager.get_shares(sec, 3);
        top.p_sboxIn1.setNode(ssec[0]);
        top.p_sboxIn2.setNode(ssec[1]);
        top.p_sboxIn3.setNode(ssec[2]);
    #else
        top.p_sboxIn1.set<uint8_t, true>(X0);
        top.p_sboxIn2.set<uint8_t, true>(X1);
        top.p_sboxIn3.set<uint8_t, true>(X2);
    #endif

    top.p_en.set<bool, true>(true);
    manager.begin_measure();

    prepare_step(manager, top);
    // Propagate
    prepare_step(manager, top);

    manager.end_measure();
    manager.stat();

    int Q0, Q1, Q2;

    Q0 = top.p_share1.get<uint8_t>() & 0xF;
    Q1 = top.p_share2.get<uint8_t>() & 0xF;
    Q2 = top.p_share3.get<uint8_t>() & 0xF;

    int Q = Q0 ^ Q1 ^Q2;
    printf("X: %d\n", X);
    printf("Q: %d\n", Q);
    printf("X0: %d\n", X0);
    printf("X1: %d\n", X1);
    printf("Q0: %d\n", Q0);
    printf("Q1: %d\n", Q1);

    if (Y != Q)
        printf("Error. \n\n");
    else
        printf("OK. \n\n");

    assert(Y == Q);
    
    std::cout << std::endl << "Unrolled " << manager.get_steps() << " steps." << std::endl;
}

