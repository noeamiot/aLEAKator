#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_and.h"
#include "manager.h"

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::GADGET, "and");
    cxxrtl_design::p_top top;
    Manager manager(top, config);
    manager.begin_measure();

    // Verif msi init
    Node& a = symbol("a", 'S', 1);
    Node& b = symbol("b", 'S', 1);

    top.p_a.setNode(manager.get_shares(a, 1)[0]);
    top.p_b.setNode(manager.get_shares(b, 1)[0]);
    // end verif msi init

    for(int steps=0; steps<4; ++steps) {

        top.p_a.set<bool>(steps & 0x1);
        top.p_b.set<bool>(steps & 0x2);

        // For simple examples, we can directly call the manager step function
        manager.step(top);

        bool p_c = top.p_c.get<bool>();
        bool p_a = top.p_a.get<bool>();
        bool p_b = top.p_b.get<bool>();

        std::cout << "c: " << p_c << std::endl;
        std::cout << "a: " << p_a << std::endl;
        std::cout << "b: " << p_b << std::endl;

        // Res is the node assembled result
        std::cout << "c : " << *top.p_c.getNode() << std::endl;
        std::cout << "a & b : " << (a & b) << std::endl;
//        checkTpsResult(*top.p_c.getNode(), true);
        if (equivalence((*top.p_c.getNode()), (a & b)))
            std::cout << "Result matches wanted expression" << std::endl;
        else
            std::cout << "Result DOES NOT match wanted expression" << std::endl;

        std::cout << std::endl << std::endl << "------------------------" << std::endl << std::endl;
    }

    std::cout << std::endl << "Unrolled " << manager.get_steps() << " steps." << std::endl;
    manager.end_measure();
    manager.stat();
}

