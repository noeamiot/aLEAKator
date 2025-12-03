#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_and_dom_wo_reg.h"
#include "manager.h"

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::GADGET, "and_dom_wo_reg");
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

    for(int steps=0; steps<16; ++steps) {

        top.p_a0.set<bool>(steps & 0x1);
        top.p_a1.set<bool>(steps & 0x2);
        top.p_b0.set<bool>(steps & 0x3);
        top.p_b1.set<bool>(steps & 0x4);
        //top.p_z.set<bool>(steps & 0x5);

        // For simple examples, we can directly call the manager step function
        manager.step(top);

        std::cout << "C0: " << top.p_c0.get<bool>() << std::endl;
        std::cout << "C1: " << top.p_c1.get<bool>() << std::endl;
        std::cout << "A0: " << top.p_a0.get<bool>() << std::endl;
        std::cout << "B0: " << top.p_b0.get<bool>() << std::endl;
        std::cout << "A1: " << top.p_a1.get<bool>() << std::endl;
        std::cout << "B1: " << top.p_b1.get<bool>() << std::endl;
        std::cout << "Z: "  << top.p_z.get<bool>()  << std::endl;

        // In case there are combinatorial wires
        if (top.p_c0.getNode() == nullptr || top.p_c1.getNode() == nullptr) {
            std::cerr << "Not displaying result nodes as they have not been propagated (yet ?)" << std::endl;
            continue;
        }

        std::cout << "c : " << (*top.p_c0.getNode() ^ *top.p_c1.getNode()) << std::endl;
        std::cout << "a & b : " << (a & b) << std::endl;

        if (equivalence((*top.p_c0.getNode() ^ *top.p_c1.getNode()), (a & b)))
            std::cout << "Result matches wanted expression" << std::endl;
        else
            std::cout << "Result DOES NOT match wanted expression" << std::endl;

        std::cout << "------------------------" << std::endl << std::endl;
    }

    std::cout << std::endl << "Unrolled " << manager.get_steps() << " steps." << std::endl;
    manager.end_measure();
    manager.stat();
}
