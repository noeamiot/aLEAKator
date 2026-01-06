#include "dom_and_setup.h"

void dom_and::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();
    a = &symbol("a", 'S', 32);
    b = &symbol("b", 'S', 32);
    m = &symbol("m", 'M', 32);

    std::cout << "Will insert a0 in index: " << this->get_index("ram", "as0") << std::endl;
    std::cout << "Will insert a1 in index: " << this->get_index("ram", "as1") << std::endl;
    std::cout << "Will insert b0 in index: " << this->get_index("ram", "bs0") << std::endl;
    std::cout << "Will insert b1 in index: " << this->get_index("ram", "bs1") << std::endl;
    std::cout << "Will insert m in index: " << this->get_index("ram", "m") << std::endl;

    std::vector<Node*> sa = getPseudoShares(*a, 2);
    std::vector<Node*> sb = getPseudoShares(*b, 2);

    cxxrtl::value<32> a0 = top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "as0")];
    cxxrtl::value<32> a1 = top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "as1")];
    cxxrtl::value<32> b0 = top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "bs0")];
    cxxrtl::value<32> b1 = top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "bs1")];
    cxxrtl::value<32> m0 = top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "m")];

    a0.setNode(sa[0]);
    a1.setNode(sa[1]);
    b0.setNode(sb[0]);
    b1.setNode(sb[1]);
    m0.setNode(m);

    symbol_to_conc_.insert({m, &constant(m0.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({sa[1], &constant(a1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({sb[1], &constant(b1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({a, &constant(a0.get<uint32_t>() ^ a1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({b, &constant(b0.get<uint32_t>() ^ b1.get<uint32_t>(), 32)});

    top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem.update(this->get_index("ram", "as0"), a0, cxxrtl::value<32>{0xFFFFFFFFu});
    top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem.update(this->get_index("ram", "as1"), a1, cxxrtl::value<32>{0xFFFFFFFFu});
    top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem.update(this->get_index("ram", "bs0"), b0, cxxrtl::value<32>{0xFFFFFFFFu});
    top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem.update(this->get_index("ram", "bs1"), b1, cxxrtl::value<32>{0xFFFFFFFFu});
    top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem.update(this->get_index("ram", "m"), m0, cxxrtl::value<32>{0xFFFFFFFFu});
}

void dom_and::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();
    std::cout << "Will retreive res0 in index: " << this->get_index("ram", "res0") << std::endl;
    std::cout << "Will retreive res1 in index: " << this->get_index("ram", "res1") << std::endl;

    Node* res0 = &simplify(*top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "res0")].getNode());
    Node* res1 = &simplify(*top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "res1")].getNode());

    std::cout << "Conc res0: " << std::hex << top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "res0")].get<uint32_t>() << std::dec << std::endl;
    std::cout << "Conc res1: " << std::hex << top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "res1")].get<uint32_t>() << std::dec << std::endl;

    std::cout << "Res 0: " << simplify(*res0) << std::endl;
    std::cout << "Res 1: " << simplify(*res1) << std::endl;

    //if (equivalence(simplify(*res0 ^ *res1), simplify(getBitDecomposition(*a & *b))))
    std::cout << "Res : " << simplify(*res0 ^ *res1).verbatimPrint() << std::endl;
    std::cout << "Res (voulu) : " << simplify(*a & *b).verbatimPrint() << std::endl;
    //if (equivalence(simplify(*res0 ^ *res1), simplify(*a & *b)))
    if (equivalence(simplify(*res0 ^ *res1), simplify(getBitDecomposition(*a & *b))))
        std::cout << "Result matches wanted expression" << std::endl;
    else
        std::cout << "Result DOES NOT match wanted expression" << std::endl;

    if (&getExpValue(getBitDecomposition(*res0), symbol_to_conc_) == &constant(top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "res0")].get<uint32_t>(), 32)
        and &getExpValue(getBitDecomposition(*res1), symbol_to_conc_) == &constant(top.memory_p_u__top__ram_2e_m__ram_2e_m__ram_2e_r__mem[this->get_index("ram", "res1")].get<uint32_t>(), 32))
        std::cout << "Concretizing result expressions provides the correct concrete result !" << std::endl;
    else
        std::cout << "Concretizing result expressions DOES NOT provide the correct concrete result, something went wrong." << std::endl;
}

void dom_and::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
