#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_prince_ti_secure.h"
#include "manager.h"

bool prepare_step(Manager& manager, cxxrtl_design::p_top& top) {
    top.prev_p_i__clk.set<bool, true>(false);
    top.p_i__clk.set<bool, true>(true);

    #ifdef CONST_EXEC
    //TODO: Set a val
    //top.p_i__r.set<uint64_t, true>(0x012345678928u);
    #else
    //top.p_i__r.set<uint64_t>(0x012345678928u);
    top.p_i__r.setNode(&symbol(("m" + std::to_string(manager.get_steps())).c_str(), 'M', 192));
    top.p_i__r.set_fully_unstable();
    #endif

    std::cout << "Curr SubRound number: " << top.p_subrnd__cnt.curr << std::endl;
    std::cout << "Curr round number: " << top.p_rnd__cnt.curr << std::endl;

    return manager.step(top);
}

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::ACCELERATOR, "prince_ti_secure");
    cxxrtl_design::p_top top;
    Manager manager(top, config);

    // Prologue
    top.p_i__reset.set<bool, true>(1);
    top.p_i__reset.set_fully_stable();
    prepare_step(manager, top);
    prepare_step(manager, top);
    manager.begin_measure();
    top.p_i__reset.set<bool, true>(0);
    top.p_i__reset.set_fully_stable();

    top.p_i__enc__dec.set<bool, true>(0);
    top.p_i__enc__dec.set_fully_stable();

    auto key1b = cxxrtl::value<64>(0x56b4cf12u, 0x93d7168du);
    auto key1t = cxxrtl::value<64>(0xddabce8eu, 0xa8366ca7u);
    auto key2b = cxxrtl::value<64>(0x20e0fd02u, 0x6d0bac15u);
    auto key2t = cxxrtl::value<64>(0xddabce8eu, 0xa8366ca7u);
    auto pt1 = cxxrtl::value<64>(0xbeb23314u, 0x4a3d8ed7u);
    auto pt2 = cxxrtl::value<64>(0x3719fefbu, 0x4b1ecbb0u);

    top.p_i__key1.slice<63, 0>() = key1b;
    top.p_i__key1.slice<127, 64>() = key1t;
    top.p_i__key2.slice<63, 0>() = key2b;
    top.p_i__key2.slice<127, 64>() = key2t;
    top.p_i__pt1 = pt1; 
    top.p_i__pt2 = pt2;

    #ifndef CONST_EXEC 
    Node* spt = &symbol("pt", 'S', 64);
    Node* sk = &symbol("k0", 'S', 128);

    std::vector<Node*> sspt = manager.get_shares(*spt, 2);
    std::vector<Node*> ssk = manager.get_shares(*sk, 2);

    top.p_i__pt1.setNode(sspt[0]);
    top.p_i__pt2.setNode(sspt[1]);

    top.p_i__key1.setNode(ssk[0]);
    top.p_i__key2.setNode(ssk[1]);

    top.p_i__pt1.set_fully_unstable();
    top.p_i__pt2.set_fully_unstable();
    top.p_i__key1.set_fully_unstable();
    top.p_i__key2.set_fully_unstable();
    #endif

    Node* expected = &constant(0xae25ad3ca8fa9ccf, 64);

    top.p_i__load.set<bool, true>(1);
    top.p_i__start.set<bool, true>(0);
    top.p_i__load.set_fully_stable();
    top.p_i__start.set_fully_stable();
    prepare_step(manager, top);
    
    top.p_i__load.set<bool, true>(0);
    top.p_i__start.set<bool, true>(1);
    top.p_i__load.set_fully_stable();
    top.p_i__start.set_fully_stable();
    // End prologue
    prepare_step(manager, top);

    while (top.p_o__done.get<uint64_t>() != 1 && manager.get_steps() < 100) {
        prepare_step(manager, top);
    }

    prepare_step(manager, top);
    manager.end_measure();
    manager.stat();
    unsigned long long ct1 = top.p_o__ct1.get<uint64_t>();
    unsigned long long ct2 = top.p_o__ct2.get<uint64_t>();
    unsigned long long res = ct1 ^ ct2;
    
    std::cout << "Result : " << std::hex << res << ", expected : " << expected->cst[0] << std::dec << std::endl;
    std::cout << "Symbolic result: " << simplify(*top.p_o__ct1.getNode() ^ *top.p_o__ct2.getNode()) << std::endl << top.p_o__ct1 << std::endl << top.p_o__ct2 << std::endl << top.p_o__done << std::endl;

    std::cout << std::endl << "Unrolled " << manager.get_steps() << std::endl;
}
