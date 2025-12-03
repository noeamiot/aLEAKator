#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_triple_xor_reg.h"
#include "manager.h"

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::GADGET, "triple_xor_reg");
    cxxrtl_design::p_top top;
    Manager manager(top, config);

    Node& a = symbol("a", 'P', 1);
    Node& b = symbol("b", 'P', 1);
    Node& c = symbol("c", 'P', 1);
    Node& d = symbol("d", 'P', 1);
    top.p_a.setNode(&a);
    top.p_b.setNode(&b);
    top.p_c.setNode(&c);
    top.p_d.setNode(&d);
    top.p_clk.set<bool>(false);
    top.p_clk.setNode(&constant(0, 1));
    top.p_a.set<bool>(true);
    top.p_b.set<bool>(false);
    top.p_c.set<bool>(true);
    top.p_d.set<bool>(false);
    manager.step(top);

    top.p_clk.set<bool>(true);
    top.p_clk.setNode(&constant(1, 1));
    manager.step(top);
    top.p_clk.set<bool>(false);
    top.p_clk.setNode(&constant(0, 1));
    manager.step(top);
    top.p_clk.set<bool>(true);
    top.p_clk.setNode(&constant(1, 1));
    manager.step(top);

    Node* s = top.p_s.getNode();
    std::cout << *s << std::endl;
    leaks::print_leakage(top.p_s.ls);
 
    std::cout << std::endl << "Unrolled " << manager.get_steps() << " steps." << std::endl;
}

