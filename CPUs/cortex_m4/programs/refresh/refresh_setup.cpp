#include "refresh_setup.h"
#include "cxxrtl_cortex_m4.h"

void refresh::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();
    a = &symbol("a", 'S', 32);
    m = &symbol("m", 'M', 32);

    std::cout << "Will insert a0 in index: " << this->get_index("ram", "a0") << std::endl;
    std::cout << "Will insert a1 in index: " << this->get_index("ram", "a1") << std::endl;

    std::vector<Node*> sa = getPseudoShares(*a, 2);

    cxxrtl::value<32> a0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "a0")];
    cxxrtl::value<32> a1 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "a1")];
    cxxrtl::value<32> m0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "m")];

    a0.setNode(sa[0]);
    a1.setNode(sa[1]);
    m0.setNode(m);

    symbol_to_conc_.insert({sa[1], &constant(a1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({a, &constant(a0.get<uint32_t>() ^ a1.get<uint32_t>(), 32)});

    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "a0"), a0, cxxrtl::value<32>{0xFFFFFFFFu});
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "a1"), a1, cxxrtl::value<32>{0xFFFFFFFFu});
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "m"), m0, cxxrtl::value<32>{0xFFFFFFFFu});
}

void refresh::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();
    std::cout << "Not checking result." << std::endl;
}

void refresh::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
