#include "simple_setup.h"

void simple::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.begin_measure();
    a = &symbol("a", 'S', 32);

    std::cout << "Will insert a0 GPR 8: " << std::endl;
    std::cout << "Will insert a1 GPR 9: " << std::endl;

    std::vector<Node*> sa = getPseudoShares(*a, 2);

    cxxrtl::value<32> a0{0xFFFFFFFFu};
    cxxrtl::value<32> a1{0xFFFFFFFFu};

    a0.setNode(sa[0]);
    a1.setNode(sa[1]);

    symbol_to_conc_.insert({sa[1], &constant(a1.get<uint32_t>(), 32)});
    symbol_to_conc_.insert({a, &constant(a0.get<uint32_t>() ^ a1.get<uint32_t>(), 32)});

    top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__8.curr = a0;
    top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__9.curr = a1;
}

void simple::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    manager.end_measure();

//    std::cout << "GPR 8: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__8.curr.node->verbatimPrint() << std::endl;
//    std::cout << "GPR 9: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__9.curr.node->verbatimPrint() << std::endl;
//    std::cout << "GPR 10: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__10.curr.node->verbatimPrint() << std::endl;
//    std::cout << "GPR 11: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__11.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 31: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__31.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 30: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__30.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 29: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__29.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 28: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__28.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 27: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__27.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 26: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__26.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 25: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__25.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 24: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__24.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 23: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__23.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 22: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__22.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 21: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__21.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 20: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__20.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 19: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__19.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 18: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__18.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 17: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__17.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 16: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__16.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 15: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__15.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 14: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__14.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 13: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__13.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 12: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__12.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 11: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__11.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 10: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__10.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 9: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__9.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 8: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__8.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 7: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__7.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 6: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__6.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 5: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__5.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 4: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__4.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 3: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__3.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 2: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__2.curr.node->verbatimPrint() << std::endl;
    std::cout << "GPR 1: " << top.p_u__top_2e_m__pipe_2e_m__back_2e_m__gpr_2e_r__gpr__0__1.curr.node->verbatimPrint() << std::endl;
}

void simple::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {

}
