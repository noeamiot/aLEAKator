#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_and_dom_5d.h"
#include "manager.h"

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::GADGET, "and_dom_5d");
    cxxrtl_design::p_top top;
    Manager manager(top, config);
    manager.begin_measure();

    // Verif msi init
    Node& a = symbol("a", 'S', 1);
    Node& b = symbol("b", 'S', 1);

    std::vector<Node*> as = manager.get_shares(a, 6);
    std::vector<Node*> bs = manager.get_shares(b, 6);

    top.p_a0.setNode(as[0]);
    top.p_a1.setNode(as[1]);
    top.p_a2.setNode(as[2]);
    top.p_a3.setNode(as[3]);
    top.p_a4.setNode(as[4]);
    top.p_a5.setNode(as[5]);

    top.p_b0.setNode(bs[0]);
    top.p_b1.setNode(bs[1]);
    top.p_b2.setNode(bs[2]);
    top.p_b3.setNode(bs[3]);
    top.p_b4.setNode(bs[4]);
    top.p_b5.setNode(bs[5]);

    top.p_z10.setNode(&symbol("z10", 'M', 1));

    top.p_z20.setNode(&symbol("z20", 'M', 1));
    top.p_z21.setNode(&symbol("z21", 'M', 1));

    top.p_z30.setNode(&symbol("z30", 'M', 1));
    top.p_z31.setNode(&symbol("z31", 'M', 1));
    top.p_z32.setNode(&symbol("z32", 'M', 1));

    top.p_z40.setNode(&symbol("z40", 'M', 1));
    top.p_z41.setNode(&symbol("z41", 'M', 1));
    top.p_z42.setNode(&symbol("z42", 'M', 1));
    top.p_z43.setNode(&symbol("z43", 'M', 1));

    top.p_z50.setNode(&symbol("z50", 'M', 1));
    top.p_z51.setNode(&symbol("z51", 'M', 1));
    top.p_z52.setNode(&symbol("z52", 'M', 1));
    top.p_z53.setNode(&symbol("z53", 'M', 1));
    top.p_z54.setNode(&symbol("z54", 'M', 1));
    // end verif msi init

    top.prev_p_clk.set<bool, true>(false);
    top.p_clk.set<bool, true>(true);

    manager.step(top);
    manager.step(top);

    std::cout << *top.p_c0.getNode() << std::endl;
    std::cout << *top.p_c1.getNode() << std::endl;
    std::cout << *top.p_c2.getNode() << std::endl;
    std::cout << *top.p_c3.getNode() << std::endl;
    std::cout << *top.p_c4.getNode() << std::endl;
    std::cout << *top.p_c5.getNode() << std::endl;

    if (equivalence(simplify(
        *top.p_c0.getNode() ^
        *top.p_c1.getNode() ^
        *top.p_c2.getNode() ^
        *top.p_c3.getNode() ^
        *top.p_c4.getNode() ^
        *top.p_c5.getNode()
    ), (a & b)))
        std::cout << "Result matches wanted expression" << std::endl;
    else
        std::cout << "Result DOES NOT match wanted expression" << std::endl;

    std::cout << std::endl << "Unrolled " << manager.get_steps() << " steps." << std::endl;
    manager.end_measure();
    manager.stat();
}
