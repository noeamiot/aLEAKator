#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_prover-keccak_sbox_dom_d1.h"
#include "manager.h"

int sbox[32] = {0x0, 0x9, 0x12, 0xB, 0x5, 0xC, 0x16, 0xF, 0xA, 0x3, 0x18, 0x1, 0xD, 0x4, 0x1E, 0x7, 0x14, 0x15, 0x6, 0x17, 0x11, 0x10, 0x2, 0x13, 0x1A, 0x1B, 0x8, 0x19, 0x1D, 0x1C, 0xE, 0x1F};

bool prepare_step(Manager& manager, cxxrtl_design::p_top& top) {
    // WARNING: ALWAYS SIMULATE THAT WE HAVE DONE A NEGEDGE BEFORE
    top.prev_p_ClkxCI.set<bool, true>(false);
    top.p_ClkxCI.set<bool, true>(true);

    return manager.step(top);
}

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::ACCELERATOR, "prover-keccak_sbox_dom_d1");
    cxxrtl_design::p_top top;
    Manager manager(top, config);

//    top.p_RstxRBI.set<bool, true>(false);
//    prepare_step(manager, top);
//    prepare_step(manager, top);
    top.p_RstxRBI.set<bool, true>(true);

    int X, Y;
    X = (rand() + 1) & 0x1F;
    Y = sbox[X];

    int X0, X1;
    X0 = rand() & 0x1F;
    X1 = X ^ X0;

    int XxDI = X0 ^ (X1 << 5);

    #ifndef CONST_EXEC
	    top.p_InputxDI.set<uint16_t>(XxDI);

	    Node& sec = symbol("sec", 'S', 5);
	    std::vector<Node*> ssec = manager.get_shares(sec, 2);
        top.p_InputxDI.setNode(&Concat(*ssec[0], *ssec[1]));
        top.p_ZxDI.setNode(&symbol("z", 'M', 5));
    #else
        top.p_InputxDI.set<uint16_t, true>(X);
    #endif

    manager.begin_measure();

    prepare_step(manager, top);
    // Propagate
    prepare_step(manager, top);

    manager.end_measure();
    std::cout << "Q: " << top.p_OutputxDO.node->verbatimPrint() << std::endl;
    manager.stat();

    int Q0, Q1;

    Q0 = top.p_OutputxDO.get<uint16_t>() & 0x1F;
    Q1 = (top.p_OutputxDO.get<uint16_t>() >> 5) & 0x1F;

    int Q = Q0 ^ Q1;
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

