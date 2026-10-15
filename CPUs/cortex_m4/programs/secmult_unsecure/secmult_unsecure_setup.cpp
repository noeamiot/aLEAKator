#include "secmult_unsecure_setup.h"
#include "cxxrtl_cortex_m4.h"

void secmult_unsecure::initial_init_implem(Manager& manager) {
    a = &symbol("a", 'S', 8);
    b = &symbol("b", 'S', 8);
    m = &symbol("m", 'M', 8);

    sa = manager.get_shares(*a, 2);
    sb = manager.get_shares(*b, 2);
}

void secmult_unsecure::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();

    cxxrtl::value<32> a0 = ram(top)[get_index("ram", "a0")];
    cxxrtl::value<32> a1 = ram(top)[get_index("ram", "a1")];
    cxxrtl::value<32> b0 = ram(top)[get_index("ram", "b0")];
    cxxrtl::value<32> b1 = ram(top)[get_index("ram", "b1")];
    cxxrtl::value<32> m0 = ram(top)[get_index("ram", "m01")];

    // Conc resut will be right because those are on 32 bits
    a0.setNode(&simplify(Concat(Const(0x0, 24), *sa[0]) << (get_position("a0") * 8)));
    a1.setNode(&simplify(Concat(Const(0x0, 24), *sa[1]) << (get_position("a1") * 8)));
    b0.setNode(&simplify(Concat(Const(0x0, 24), *sb[0]) << (get_position("b0") * 8)));
    b1.setNode(&simplify(Concat(Const(0x0, 24), *sb[1]) << (get_position("b1") * 8)));
    m0.setNode(&simplify(Concat(Const(0x0, 24), *m) << (get_position("m01") * 8)));

    symbol_to_conc_.insert({m, &constant((m0.get<uint32_t>() >> (get_position("m01")*8)) & 0xFFu, 8)});
    symbol_to_conc_.insert({sa[1], &constant((a1.get<uint32_t>() >> (get_position("a1")*8)) & 0xFFu, 8)});
    symbol_to_conc_.insert({sb[1], &constant((b1.get<uint32_t>() >> (get_position("b1")*8)) & 0xFFu, 8)});
    symbol_to_conc_.insert({a, &constant((((a0.get<uint32_t>() >> (get_position("a0")*8)) & 0xFFu) ^ (a1.get<uint32_t>() >> (get_position("a1")*8))) & 0xFFu, 8)});
    symbol_to_conc_.insert({b, &constant((((b0.get<uint32_t>() >> (get_position("b0")*8)) & 0xFFu) ^ (b1.get<uint32_t>() >> (get_position("b1")*8))) & 0xFFu, 8)});

    ram(top).update(get_index("ram", "a0"), a0, get_mask("a0"));
    ram(top).update(get_index("ram", "a1"), a1, get_mask("a1"));
    ram(top).update(get_index("ram", "b0"), b0, get_mask("b0"));
    ram(top).update(get_index("ram", "b1"), b1, get_mask("b1"));
    ram(top).update(get_index("ram", "m01"), m0, get_mask("m01"));
}

void secmult_unsecure::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();

    Node* c0 = &simplify(Extract((get_position("c0")+1)*8-1, get_position("c0")*8, *ram(top)[get_index("ram", "c0")].getNode()));
    Node* c1 = &simplify(Extract((get_position("c1")+1)*8-1, get_position("c1")*8, *ram(top)[get_index("ram", "c1")].getNode()));

    uint32_t rc0 = ((ram(top)[get_index("ram", "c0")].get<uint32_t>() >> (get_position("c0")*8)) & 0xFFu);
    uint32_t rc1 = ((ram(top)[get_index("ram", "c1")].get<uint32_t>() >> (get_position("c1")*8)) & 0xFFu);

    std::cout << "Conc c0: " << std::hex << rc0 << std::dec << std::endl;
    std::cout << "Conc c1: " << std::hex << rc1 << std::dec << std::endl;

    std::cout << "Res 0: " << simplify(*c0) << std::endl;
    std::cout << "Res 1: " << simplify(*c1) << std::endl;

    if (&getExpValue(getBitDecomposition(*c0), symbol_to_conc_) == &constant(rc0, 8)
        and &getExpValue(getBitDecomposition(*c1), symbol_to_conc_) == &constant(rc1, 8))
        std::cout << "Concretizing result expressions provides the correct concrete result !" << std::endl;
    else
        std::cout << "Concretizing result expressions DOES NOT provide the correct concrete result, something went wrong." << std::endl;
}

void secmult_unsecure::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
