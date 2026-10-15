#include "secmult_setup.h"
#include "cxxrtl_cv32e40p.h"

void secmult::initial_init_implem(Manager& manager) {
    a = &symbol("a", 'S', 8);
    b = &symbol("b", 'S', 8);
    m = &symbol("m", 'M', 8);

    sa = manager.get_shares(*a, 2);
    sb = manager.get_shares(*b, 2);
}

void secmult::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();

    cxxrtl::value<32> a0 = ram(top)[get_index("ram", "a0")];
    cxxrtl::value<32> a1 = ram(top)[get_index("ram", "a1")];
    cxxrtl::value<32> b0 = ram(top)[get_index("ram", "b0")];
    cxxrtl::value<32> b1 = ram(top)[get_index("ram", "b1")];
    cxxrtl::value<32> m0 = ram(top)[get_index("ram", "m01")];

    // Conc resut will be right because those are on 32 bits
    a0.setNode(&Concat(Const(0x0, 24), *sa[0]));
    a1.setNode(&Concat(Const(0x0, 24), *sa[1]));
    b0.setNode(&Concat(Const(0x0, 24), *sb[0]));
    b1.setNode(&Concat(Const(0x0, 24), *sb[1]));
    m0.setNode(&Concat(Const(0x0, 24), *m));

    symbol_to_conc_.insert({m, &constant(m0.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({sa[1], &constant(a1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({sb[1], &constant(b1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({a, &constant(a0.get<uint32_t>() ^ a1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({b, &constant(b0.get<uint32_t>() ^ b1.get<uint32_t>(), 32)});

    ram(top).update(get_index("ram", "a0"), a0, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "a1"), a1, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "b0"), b0, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "b1"), b1, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "m01"), m0, cxxrtl::value<32>{0xFFFFFFFFu});
}

void secmult::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();

    Node* c0 = &simplify(*ram(top)[get_index("ram", "c0")].getNode());
    Node* c1 = &simplify(*ram(top)[get_index("ram", "c1")].getNode());

    std::cout << "Conc c0: " << std::hex << ram(top)[get_index("ram", "c0")].get<uint32_t>() << std::dec << std::endl;
    std::cout << "Conc c1: " << std::hex << ram(top)[get_index("ram", "c1")].get<uint32_t>() << std::dec << std::endl;

    std::cout << "Res 0: " << simplify(*c0) << std::endl;
    std::cout << "Res 1: " << simplify(*c1) << std::endl;

    if (&getExpValue(getBitDecomposition(*c0), symbol_to_conc_) == &constant(ram(top)[get_index("ram", "c0")].get<uint32_t>(), 32)
        and &getExpValue(getBitDecomposition(*c1), symbol_to_conc_) == &constant(ram(top)[get_index("ram", "c1")].get<uint32_t>(), 32))
        std::cout << "Concretizing result expressions provides the correct concrete result !" << std::endl;
    else
        std::cout << "Concretizing result expressions DOES NOT provide the correct concrete result, something went wrong." << std::endl;
}

void secmult::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
