#include "secmult_unsecure_setup.h"
#include "cxxrtl_cortex_m3.h"

void secmult_unsecure::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();
    a = &symbol("a", 'S', 8);
    b = &symbol("b", 'S', 8);
    Node* m = &symbol("m", 'M', 8);

    std::cout << "Will insert a0 in index: " << this->get_index("ram", "a0") << std::endl;
    std::cout << "Will insert a1 in index: " << this->get_index("ram", "a1") << std::endl;
    std::cout << "Will insert b0 in index: " << this->get_index("ram", "b0") << std::endl;
    std::cout << "Will insert b1 in index: " << this->get_index("ram", "b1") << std::endl;
    std::cout << "Will insert m01 in index: " << this->get_index("ram", "m01") << std::endl;

    std::vector<Node*> sa = getPseudoShares(*a, 2);
    std::vector<Node*> sb = getPseudoShares(*b, 2);

    cxxrtl::value<32> a0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "a0")];
    cxxrtl::value<32> a1 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "a1")];
    cxxrtl::value<32> b0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "b0")];
    cxxrtl::value<32> b1 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "b1")];
    cxxrtl::value<32> m0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "m01")];

    // Conc resut will be right because those are on 32 bits
    a0.setNode(&simplify(Concat(Const(0x0, 24), *sa[0]) << (this->get_position("a0") * 8)));
    a1.setNode(&simplify(Concat(Const(0x0, 24), *sa[1]) << (this->get_position("a1") * 8)));
    b0.setNode(&simplify(Concat(Const(0x0, 24), *sb[0]) << (this->get_position("b0") * 8)));
    b1.setNode(&simplify(Concat(Const(0x0, 24), *sb[1]) << (this->get_position("b1") * 8)));
    m0.setNode(&simplify(Concat(Const(0x0, 24), *m) << (this->get_position("m01") * 8)));

    symbol_to_conc_.insert({m, &constant((m0.get<uint32_t>() >> (this->get_position("m01")*8)) & 0xFFu, 8)});
    symbol_to_conc_.insert({sa[1], &constant((a1.get<uint32_t>() >> (this->get_position("a1")*8)) & 0xFFu, 8)});
    symbol_to_conc_.insert({sb[1], &constant((b1.get<uint32_t>() >> (this->get_position("b1")*8)) & 0xFFu, 8)});
    symbol_to_conc_.insert({a, &constant((((a0.get<uint32_t>() >> (this->get_position("a0")*8)) & 0xFFu) ^ (a1.get<uint32_t>() >> (this->get_position("a1")*8))) & 0xFFu, 8)});
    symbol_to_conc_.insert({b, &constant((((b0.get<uint32_t>() >> (this->get_position("b0")*8)) & 0xFFu) ^ (b1.get<uint32_t>() >> (this->get_position("b1")*8))) & 0xFFu, 8)});

    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "a0"), a0, this->get_mask("a0"));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "a1"), a1, this->get_mask("a1"));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "b0"), b0, this->get_mask("b0"));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "b1"), b1, this->get_mask("b1"));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "m01"), m0, this->get_mask("m01"));
}

void secmult_unsecure::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();
    std::cout << "Will retreive c0 in index: " << this->get_index("ram", "c0") << std::endl;
    std::cout << "Will retreive c1 in index: " << this->get_index("ram", "c1") << std::endl;

    Node* c0 = &simplify(Extract((this->get_position("c0")+1)*8-1, this->get_position("c0")*8, *top.memory_p_uRAM_2e_RAM[this->get_index("ram", "c0")].getNode()));
    Node* c1 = &simplify(Extract((this->get_position("c1")+1)*8-1, this->get_position("c1")*8, *top.memory_p_uRAM_2e_RAM[this->get_index("ram", "c1")].getNode()));

    uint32_t rc0 = ((top.memory_p_uRAM_2e_RAM[this->get_index("ram", "c0")].get<uint32_t>() >> (this->get_position("c0")*8)) & 0xFFu);
    uint32_t rc1 = ((top.memory_p_uRAM_2e_RAM[this->get_index("ram", "c1")].get<uint32_t>() >> (this->get_position("c1")*8)) & 0xFFu);

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
