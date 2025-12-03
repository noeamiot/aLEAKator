#include "secmult_setup.h"
#include "cxxrtl_coco-ibex.h"

void secmult::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
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

    cxxrtl::value<32> a0 = (*ram[this->get_index("ram", "a0")]).curr;
    cxxrtl::value<32> a1 = (*ram[this->get_index("ram", "a1")]).curr;
    cxxrtl::value<32> b0 = (*ram[this->get_index("ram", "b0")]).curr;
    cxxrtl::value<32> b1 = (*ram[this->get_index("ram", "b1")]).curr;
    cxxrtl::value<32> m0 = (*ram[this->get_index("ram", "m01")]).curr;

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

    (*ram[this->get_index("ram", "a0")]).curr = a0;
    (*ram[this->get_index("ram", "a1")]).curr = a1;
    (*ram[this->get_index("ram", "b0")]).curr = b0;
    (*ram[this->get_index("ram", "b1")]).curr = b1;
    (*ram[this->get_index("ram", "m01")]).curr = m0;
}

void secmult::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();
    std::cout << "Will retreive c0 in index: " << this->get_index("ram", "c0") << std::endl;
    std::cout << "Will retreive c1 in index: " << this->get_index("ram", "c1") << std::endl;

    Node* c0 = &simplify(*(*ram[this->get_index("ram", "c0")]).curr.getNode());
    Node* c1 = &simplify(*(*ram[this->get_index("ram", "c1")]).curr.getNode());

    std::cout << "Conc c0: " << std::hex << (*ram[this->get_index("ram", "c0")]).curr.get<uint32_t>() << std::dec << std::endl;
    std::cout << "Conc c1: " << std::hex << (*ram[this->get_index("ram", "c1")]).curr.get<uint32_t>() << std::dec << std::endl;
    std::cout << "Conc res: " << std::hex << ((*ram[this->get_index("ram", "c1")]).curr.get<uint32_t>() ^ (*ram[this->get_index("ram", "c0")]).curr.get<uint32_t>()) << std::dec << std::endl;

    std::cout << "Res 0: " << simplify(*c0) << std::endl;
    std::cout << "Res 1: " << simplify(*c1) << std::endl;

    if (&getExpValue(getBitDecomposition(*c0), symbol_to_conc_) == &constant((*ram[this->get_index("ram", "c0")]).curr.get<uint32_t>(), 32)
        and &getExpValue(getBitDecomposition(*c1), symbol_to_conc_) == &constant((*ram[this->get_index("ram", "c1")]).curr.get<uint32_t>(), 32))
        std::cout << "Concretizing result expressions provides the correct concrete result !" << std::endl;
    else
        std::cout << "Concretizing result expressions DOES NOT provide the correct concrete result, something went wrong." << std::endl;
}

void secmult::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
