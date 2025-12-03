#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_and_dom_sync.h"
#include "manager.h"

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::GADGET, "and_dom_sync");
    cxxrtl_design::p_top top;
    Manager manager(top, config);
    manager.begin_measure();

    // Verif msi init
    Node& a = symbol("a", 'S', 1);
    Node& b = symbol("b", 'S', 1);

    std::vector<Node*> as = manager.get_shares(a, 2);
    std::vector<Node*> bs = manager.get_shares(b, 2);

    top.p_a0.setNode(as[0]);
    top.p_a1.setNode(as[1]);
    top.p_b0.setNode(bs[0]);
    top.p_b1.setNode(bs[1]);
    top.p_z.setNode(&symbol("z", 'M', 1));
    // end verif msi init

    top.prev_p_clk.set<bool, true>(false);
    top.p_clk.set<bool, true>(true);

    manager.step(top);
    manager.step(top);

    std::cout << *top.p_c0.getNode() << std::endl;
    std::cout << *top.p_c1.getNode() << std::endl;

    if (equivalence((*top.p_c0.getNode() ^ *top.p_c1.getNode()), (a & b)))
        std::cout << "Result matches wanted expression" << std::endl;
    else
        std::cout << "Result DOES NOT match wanted expression" << std::endl;

    std::cout << std::endl << "Unrolled " << manager.get_steps() << " steps." << std::endl;
    manager.end_measure();
    manager.stat();
}
