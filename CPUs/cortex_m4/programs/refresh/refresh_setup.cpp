#include "refresh_setup.h"
#include "cxxrtl_cortex_m4.h"

void refresh::initial_init_implem(Manager& manager) {
    a = &symbol("a", 'S', 32);
    m = &symbol("m", 'M', 32);

    sa = manager.get_shares(*a, 2);
}

void refresh::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();

    cxxrtl::value<32> a0 = ram(top)[get_index("ram", "a0")];
    cxxrtl::value<32> a1 = ram(top)[get_index("ram", "a1")];
    cxxrtl::value<32> m0 = ram(top)[get_index("ram", "m")];

    a0.setNode(sa[0]);
    a1.setNode(sa[1]);
    m0.setNode(m);

    symbol_to_conc_.insert({sa[1], &constant(a1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({a, &constant(a0.get<uint32_t>() ^ a1.get<uint32_t>(), 32)});

    ram(top).update(get_index("ram", "a0"), a0, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "a1"), a1, cxxrtl::value<32>{0xFFFFFFFFu});
    ram(top).update(get_index("ram", "m"), m0, cxxrtl::value<32>{0xFFFFFFFFu});
}

void refresh::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();
    std::cout << "Not checking result." << std::endl;
}

void refresh::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
