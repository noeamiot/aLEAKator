#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_dff.h"
#include "manager.h"

void flip(Manager& manager, cxxrtl_design::p_top& top) {
    top.p_clk.set<bool, true>(!top.p_clk.get<bool>());
    manager.step(top);
    std::cout << "clk " << top.p_clk << ", D " << top.p_D << ", Q " << *top.p_Q.getNode() << std::endl;
}

// Please note that there is nothing to verify here, this is just a test
int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::GADGET, "dff");
    cxxrtl_design::p_top top;
    Manager manager(top, config);

    top.p_D.set<bool>(true);
    top.p_D.setNode(&symbol("D", 'P', 1));
    flip(manager, top);
    flip(manager, top);
    flip(manager, top);

    top.p_D.set<bool>(false);
//    top.p_D.setNode(&constant(0, 1));
    std::cout << "D -> 0" << std::endl;
    manager.step(top);
    std::cout << "clk " << top.p_clk << ", D " << top.p_D << ", Q " << *top.p_Q.getNode() << std::endl;
    flip(manager, top);
    flip(manager, top);
    flip(manager, top);
    flip(manager, top);

    std::cout << "D -> 1" << std::endl;
    top.p_D.set<bool>(true);
//    top.p_D.setNode(&constant(1, 1));
    flip(manager, top);
    flip(manager, top);
    manager.step(top);
    std::cout << "clk " << top.p_clk << ", D " << top.p_D << ", Q " << *top.p_Q.getNode() << std::endl;
    flip(manager, top);
    flip(manager, top);
    manager.step(top);
    std::cout << "clk " << top.p_clk << ", D " << top.p_D << ", Q " << *top.p_Q.getNode() << std::endl;
    flip(manager, top);

    std::cout << std::endl << "Unrolled " << manager.get_steps() << " steps in." << std::endl;
}

