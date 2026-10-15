#include "dom_and_setup.h"
#include "cxxrtl_cortex_m4.h"

void dom_and::initial_init_implem(Manager& manager) {
    a = &symbol("a", 'S', 32);
    b = &symbol("b", 'S', 32);
    m = &symbol("m", 'M', 32);

    sa = manager.get_shares(*a, 2);
    sb = manager.get_shares(*b, 2);
}

void dom_and::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();

    cxxrtl::value<32> a0 = ram(top)[get_index("ram", "a0")];
    cxxrtl::value<32> a1 = ram(top)[get_index("ram", "a1")];
    cxxrtl::value<32> b0 = ram(top)[get_index("ram", "b0")];
    cxxrtl::value<32> b1 = ram(top)[get_index("ram", "b1")];
    cxxrtl::value<32> m0 = ram(top)[get_index("ram", "m")];

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

    ram(top).update(get_index("ram", "a0"), a0, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "a1"), a1, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "b0"), b0, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "b1"), b1, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "m"), m0, cxxrtl::value<32>{0xFFFFFFFFu});
}

void dom_and::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();

    Node* res0 = &simplify(*ram(top)[get_index("ram", "res0")].getNode());
    Node* res1 = &simplify(*ram(top)[get_index("ram", "res1")].getNode());

    std::cout << "Conc res0: " << std::hex << ram(top)[get_index("ram", "res0")].get<uint32_t>() << std::dec << std::endl;
    std::cout << "Conc res1: " << std::hex << ram(top)[get_index("ram", "res1")].get<uint32_t>() << std::dec << std::endl;

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

    if (&getExpValue(getBitDecomposition(*res0), symbol_to_conc_) == &constant(ram(top)[get_index("ram", "res0")].get<uint32_t>(), 32)
        and &getExpValue(getBitDecomposition(*res1), symbol_to_conc_) == &constant(ram(top)[get_index("ram", "res1")].get<uint32_t>(), 32))
        std::cout << "Concretizing result expressions provides the correct concrete result !" << std::endl;
    else
        std::cout << "Concretizing result expressions DOES NOT provide the correct concrete result, something went wrong." << std::endl;
}

void dom_and::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
