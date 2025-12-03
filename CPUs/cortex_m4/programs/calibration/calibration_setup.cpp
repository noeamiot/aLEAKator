#include "calibration_setup.h"
#include "cxxrtl_cortex_m4.h"

void calibration::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();
    a = &symbol("a", 'S', 32);

    std::vector<Node*> sa = getPseudoShares(*a, 2);

    cxxrtl::value<32> a0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "a0")];
    cxxrtl::value<32> a1 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "a1")];

    a0.setNode(sa[0]);
    a1.setNode(sa[1]);

    symbol_to_conc_.insert({sa[1], &constant(a1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({a, &constant(a0.get<uint32_t>() ^ a1.get<uint32_t>(), 32)});

    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "a0"), a0, cxxrtl::value<32>{0xFFFFFFFFu});
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "a1"), a1, cxxrtl::value<32>{0xFFFFFFFFu});
}

void calibration::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();
    Node* res = &simplify(*top.memory_p_uRAM_2e_RAM[this->get_index("ram", "res")].getNode());

    //if (equivalence(simplify(*res0 ^ *res1), simplify(getBitDecomposition(*a & *b))))
    std::cout << "Res : " << simplify(*res).verbatimPrint() << std::endl;
    std::cout << "Res (voulu) : " << simplify(*a).verbatimPrint() << std::endl;
    //if (equivalence(simplify(*res0 ^ *res1), simplify(*a & *b)))
    if (equivalence(simplify(*res), simplify(getBitDecomposition(*a))))
        std::cout << "Result matches wanted expression" << std::endl;
    else
        std::cout << "Result DOES NOT match wanted expression" << std::endl;

    if (&getExpValue(getBitDecomposition(*res), symbol_to_conc_) == &constant(top.memory_p_uRAM_2e_RAM[this->get_index("ram", "res")].get<uint32_t>(), 32))
        std::cout << "Concretizing result expressions provides the correct concrete result !" << std::endl;
    else
        std::cout << "Concretizing result expressions DOES NOT provide the correct concrete result, something went wrong." << std::endl;
}

void calibration::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
