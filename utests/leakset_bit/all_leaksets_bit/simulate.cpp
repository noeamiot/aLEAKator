#include <iostream>
#include "verif_msi_pp.hpp"
#include "lss.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    // Check initial creation
    Node* a = &symbol("a", 'S', 5);
    [[maybe_unused]] leaks::LeakSet* reg_init = leaks::reg_stabilize(a);

    assert(reg_init != nullptr && reg_init->leaks.size() == static_cast<size_t>(a->width));
    for (int i = 0; i < a->width; i++) {
        assert(reg_init->leaks[i].size() == 1);
        assert(*reg_init->leaks[i].begin() == &simplify(Extract(i, i, *a)));
    }

    Node* b = &symbol("b", 'S', 5);
    Node* c = &symbol("c", 'S', 5);
    leaks::LeakSet* ls_a = leaks::reg_stabilize(a);
    leaks::LeakSet* ls_b = leaks::reg_stabilize(b);
    leaks::LeakSet* ls_c = leaks::reg_stabilize(c);

    // Check bitwise merge
    leaks::LeakSet* merged = leaks::merge(ls_a, ls_b);
    assert(merged != ls_a && merged != ls_b);
    for (int i = 0; i < a->width; i++) {
        std::set<Node*> cmp{&simplify(Extract(i, i, *a)), &simplify(Extract(i, i, *b))};
        assert(merged->leaks[i].size() == 2);
        assert(merged->leaks[i] == cmp);
    }

    merged = leaks::merge(merged, ls_c);
    assert(merged != ls_c);
    for (int i = 0; i < a->width; i++) {
        std::set<Node*> cmp{&simplify(Extract(i, i, *a)), &simplify(Extract(i, i, *b)),
            &simplify(Extract(i, i, *c))};
        assert(merged->leaks[i].size() == 3);
        assert(merged->leaks[i] == cmp);
    }

    // Check mix with two elements
    std::set<Node*> mixed_cmp;
    for (int i = 0; i < a->width; i++) {
        mixed_cmp.insert(&simplify(Extract(i, i, *a)));
        mixed_cmp.insert(&simplify(Extract(i, i, *b)));
    }
    leaks::LeakSet* mixed = leaks::mix(ls_a, ls_b);
    assert(mixed != ls_a && merged != ls_b);
    for (int i = 0; i < a->width; i++) {
        assert(mixed->leaks[i].size() == static_cast<size_t>(a->width + b->width));
        assert(mixed->leaks[i] == mixed_cmp);
    }
    for (int i = 0; i < a->width; i++) {
        mixed_cmp.insert(&simplify(Extract(i, i, *c)));
    }
    mixed = leaks::mix(mixed, ls_c);
    assert(mixed != ls_c);
    for (int i = 0; i < a->width; i++) {
        assert(mixed->leaks[i].size() == static_cast<size_t>(a->width + b->width + c->width));
        assert(mixed->leaks[i] == mixed_cmp);
    }

    // Check extract
    for (int i = 0; i < a->width; i++) {
        [[maybe_unused]] leaks::LeakSet* extract_a = leaks::extract(ls_a, i, i);
        assert(extract_a != ls_a);
        assert(extract_a != nullptr && extract_a->leaks.size() == 1);
        assert(extract_a->leaks[0] == std::set<Node*>{&simplify(Extract(i, i, *a))});

        [[maybe_unused]] leaks::LeakSet* extract_merged = leaks::extract(merged, i, i);
        std::set<Node*> cmp{&simplify(Extract(i, i, *a)), &simplify(Extract(i, i, *b)),
            &simplify(Extract(i, i, *c))};
        assert(extract_merged != merged && extract_merged != merged);
        assert(extract_merged != nullptr && extract_merged->leaks.size() == 1);
        assert(extract_merged->leaks[0] == cmp);
    }

    // Check extend
    leaks::LeakSet* ls_extend = leaks::extend(ls_a, 7);
    assert(ls_extend != nullptr && ls_extend->leaks.size() == 7);
    for (int i = 0; i < a->width; i++) {
        assert(ls_extend->leaks[i].size() == 1);
        assert(*ls_extend->leaks[i].begin() == &simplify(Extract(i, i, *a)));
    }
    assert(ls_extend->leaks[5].size() == 0);
    assert(ls_extend->leaks[6].size() == 0);

    // Same size check
    ls_extend = leaks::extend(ls_extend, 7);
    assert(ls_extend != nullptr && ls_extend->leaks.size() == 7);
    for (int i = 0; i < a->width; i++) {
        assert(ls_extend->leaks[i].size() == 1);
        assert(*ls_extend->leaks[i].begin() == &simplify(Extract(i, i, *a)));
    }
    assert(ls_extend->leaks[5].size() == 0);
    assert(ls_extend->leaks[6].size() == 0);

    // Check sextend
    leaks::LeakSet* lss_extend = leaks::sextend(ls_a, 7);
    assert(lss_extend != nullptr && lss_extend->leaks.size() == 7);
    for (int i = 0; i < a->width; i++) {
        assert(lss_extend->leaks[i].size() == 1);
        assert(*lss_extend->leaks[i].begin() == &simplify(Extract(i, i, *a)));
    }
    assert(lss_extend->leaks[5].size() == 1 && *lss_extend->leaks[5].begin() == &simplify(Extract(4, 4, *a)));
    assert(lss_extend->leaks[6].size() == 1 && *lss_extend->leaks[6].begin() == &simplify(Extract(4, 4, *a)));

    // Same size check
    lss_extend = leaks::sextend(lss_extend, 7);
    assert(lss_extend != nullptr && lss_extend->leaks.size() == 7);
    for (int i = 0; i < a->width; i++) {
        assert(lss_extend->leaks[i].size() == 1);
        assert(*lss_extend->leaks[i].begin() == &simplify(Extract(i, i, *a)));
    }
    assert(lss_extend->leaks[5].size() == 1 && *lss_extend->leaks[5].begin() == &simplify(Extract(4, 4, *a)));
    assert(lss_extend->leaks[6].size() == 1 && *lss_extend->leaks[6].begin() == &simplify(Extract(4, 4, *a)));

    // Check rextend
    leaks::LeakSet* ls_rextend = leaks::rextend(ls_a, 7);
    assert(ls_rextend != nullptr && ls_rextend->leaks.size() == 7);
    assert(ls_rextend->leaks[0].size() == 0);
    assert(ls_rextend->leaks[1].size() == 0);
    for (int i = 0; i < a->width; i++) {
        assert(ls_rextend->leaks[7-a->width+i].size() == 1);
        assert(*ls_rextend->leaks[7-a->width+i].begin() == &simplify(Extract(i, i, *a)));
    }

    // Same size check
    ls_rextend = leaks::rextend(ls_rextend, 7);
    assert(ls_rextend != nullptr && ls_rextend->leaks.size() == 7);
    assert(ls_rextend->leaks[0].size() == 0);
    assert(ls_rextend->leaks[1].size() == 0);
    for (int i = 0; i < a->width; i++) {
        assert(ls_rextend->leaks[7-a->width+i].size() == 1);
        assert(*ls_rextend->leaks[7-a->width+i].begin() == &simplify(Extract(i, i, *a)));
    }

    // Check blit
    leaks::LeakSet* ls_b0 = leaks::extract(ls_b, 0, 0);
    [[maybe_unused]] leaks::LeakSet* blited = leaks::blit(ls_a, ls_b0, 0, 0, 5);
    assert(blited != nullptr && blited->leaks.size() == ls_a->leaks.size());
    assert(blited->leaks[0] == ls_b->leaks[0]);
    for (int i = 1; i < a->width; i++) {
        assert(blited->leaks[i] == ls_a->leaks[i]);
    }

    blited = leaks::blit(ls_a, ls_b0, 4, 4, 5);
    assert(blited != nullptr && blited->leaks.size() == ls_a->leaks.size());
    assert(blited->leaks[4] == ls_b->leaks[0]);
    for (int i = 0; i < a->width-1; i++) {
        assert(blited->leaks[i] == ls_a->leaks[i]);
    }

    blited = leaks::blit(ls_a, ls_b0, 3, 3, 5);
    assert(blited != nullptr && blited->leaks.size() == ls_a->leaks.size());
    assert(blited->leaks[2] == ls_a->leaks[2]);
    assert(blited->leaks[3] == ls_b->leaks[0]);
    assert(blited->leaks[4] == ls_a->leaks[4]);

    leaks::LeakSet* ls_b0_2 = leaks::extract(ls_b, 0, 1);
    blited = leaks::blit(ls_a, ls_b0_2, 0, 1, 5);
    assert(blited != nullptr && blited->leaks.size() == ls_a->leaks.size());
    assert(blited->leaks[0] == ls_b->leaks[0]);
    assert(blited->leaks[1] == ls_b->leaks[1]);
    for (int i = 2; i < a->width; i++) {
        assert(blited->leaks[i] == ls_a->leaks[i]);
    }

    blited = leaks::blit(ls_a, ls_b0_2, 3, 4, 5);
    assert(blited != nullptr && blited->leaks.size() == ls_a->leaks.size());
    assert(blited->leaks[3] == ls_b->leaks[0]);
    assert(blited->leaks[4] == ls_b->leaks[1]);
    for (int i = 0; i < a->width-2; i++) {
        assert(blited->leaks[i] == ls_a->leaks[i]);
    }

    blited = leaks::blit(ls_a, ls_b0_2, 2, 3, 5);
    assert(blited != nullptr && blited->leaks.size() == ls_a->leaks.size());
    assert(blited->leaks[1] == ls_a->leaks[1]);
    assert(blited->leaks[2] == ls_b->leaks[0]);
    assert(blited->leaks[3] == ls_b->leaks[1]);
    assert(blited->leaks[4] == ls_a->leaks[4]);

    // Check replicate
    leaks::LeakSet* ls_bit = leaks::extract(merged, 0, 0);
    [[maybe_unused]] leaks::LeakSet* ls_replicate = leaks::replicate(ls_bit, 10);
    assert(ls_replicate != nullptr && ls_replicate->leaks.size() == 10);
    for (int i = 0; i < 10; i++) {
        assert(ls_replicate->leaks[i] == ls_bit->leaks[0]);
    }
    ls_replicate = leaks::replicate(ls_bit, 1);
    assert(ls_replicate != nullptr && ls_replicate->leaks.size() == 1);
    assert(ls_replicate->leaks[0] == ls_bit->leaks[0]);

    // Check reduce
    [[maybe_unused]] leaks::LeakSet* ls_reduce = reduce(ls_a);
    std::set<Node*> red;
    red.insert(ls_a->leaks[0].begin(), ls_a->leaks[0].end());
    red.insert(ls_a->leaks[1].begin(), ls_a->leaks[1].end());
    red.insert(ls_a->leaks[2].begin(), ls_a->leaks[2].end());
    red.insert(ls_a->leaks[3].begin(), ls_a->leaks[3].end());
    red.insert(ls_a->leaks[4].begin(), ls_a->leaks[4].end());

    assert(ls_reduce != nullptr && red == ls_reduce->leaks[0]);
    assert(ls_reduce->leaks.size() == 1);

    // Check shift_right

    // Check shift_left

    // Check flatten
    std::set<Node*> flatten_cmp;
    for (int i = 0; i < a->width; i++) {
        flatten_cmp.insert(&simplify(Extract(i, i, *a)));
        flatten_cmp.insert(&simplify(Extract(i, i, *b)));
        flatten_cmp.insert(&simplify(Extract(i, i, *c)));
    }
    std::set<Node*> flattened = leaks::flatten(merged);
    assert(flattened.size() == static_cast<size_t>(a->width + b->width + c->width));
    assert(flattened == flatten_cmp);

    // Check partial stable creation with fully stable input
    uint32_t stab[1];
    stab[0] = 0b11111;
    [[maybe_unused]] leaks::LeakSet* reg_part = leaks::partial_stabilize(leaks::reg_stabilize(c), a, stab);

    assert(reg_part != nullptr && reg_part->leaks.size() == static_cast<size_t>(a->width));
    for (int i = 0; i < a->width; i++) {
        assert(reg_part->leaks[i].size() == 1);
        assert(*reg_part->leaks[i].begin() == &simplify(Extract(i, i, *a)));
    }

    stab[0] = 0b00000;
    reg_part = leaks::partial_stabilize(leaks::reg_stabilize(c), a, stab);

    assert(reg_part != nullptr && reg_part->leaks.size() == static_cast<size_t>(a->width));
    for (int i = 0; i < a->width; i++) {
        assert(reg_part->leaks[i].size() == 1);
        assert(*reg_part->leaks[i].begin() == &simplify(Extract(i, i, *c)));
    }

    stab[0] = 0b01011;
    reg_part = leaks::partial_stabilize(merged, a, stab);
    assert(reg_part != nullptr && reg_part->leaks.size() == static_cast<size_t>(a->width));
    assert(reg_part->leaks[0].size() == 1 && *reg_part->leaks[0].begin() == &simplify(Extract(0, 0, *a)));
    assert(reg_part->leaks[1].size() == 1 && *reg_part->leaks[1].begin() == &simplify(Extract(1, 1, *a)));
    assert(reg_part->leaks[2].size() == 3 && reg_part->leaks[2] == merged->leaks[2]);
    assert(reg_part->leaks[3].size() == 1 && *reg_part->leaks[3].begin() == &simplify(Extract(3, 3, *a)));
    assert(reg_part->leaks[4].size() == 3 && reg_part->leaks[4] == merged->leaks[4]);

    leaks::clear();
    verifMSICleanup();
}
