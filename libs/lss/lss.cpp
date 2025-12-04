#include "lss.h"

// If disabled, all functions will early return nullptr
#ifndef DISABLE_LEAKSETS
// Empty
#define REMOVE_DISABLED
#else
#define REMOVE_DISABLED return nullptr;
#endif

namespace leaks {
std::set<LeakSet*> LeakSet::ls_mem_ = std::set<LeakSet*>();
std::set<LeakSet*> LeakSet::ls_keep_mem_ = std::set<LeakSet*>();

// Allocates new leaksSet containing for each bit both given in arguments if they exist
LeakSet* merge(LeakSet* first, LeakSet* second) {
    REMOVE_DISABLED
    if (first != nullptr and second != nullptr) {
        assert(first->leaks.size() == second->leaks.size());
        return new LeakSet(*first, *second);
    } else if (first == nullptr and second == nullptr) {
        return nullptr;
    } else {
        // If first is not nullptr return it, otherwise return second, even if it is nullptr
        return new LeakSet((first != nullptr) ? *first : *second);
    }
}

// Allocates new leaksSet containing for each bit, all other in the vector
LeakSet* merge(const std::vector<LeakSet*>& lss) {
    REMOVE_DISABLED
    if (lss.size() == 0)
        return nullptr;

    LeakSet* out = nullptr;
    for (auto& ls : lss) {
        // Find first ls and copy it
        if (out == nullptr and ls != nullptr) {
            out = new LeakSet(ls->leaks);
        // Then populate with all other non null ls
        } else if (ls != nullptr) {
            assert(out->leaks.size() == ls->leaks.size());
            for (size_t i = 0; i < ls->leaks.size(); ++i)
                out->leaks[i].insert(ls->leaks[i].begin(), ls->leaks[i].end());
        }
        // if ls is null continue
    }

    return out;
}

// Allocates new leaksSet containing all bits of all leaksets in entry if they exist
LeakSet* mix(LeakSet* first, LeakSet* second) {
    REMOVE_DISABLED
    if (first == nullptr and second == nullptr) return nullptr;
    if (first != nullptr and second != nullptr)
        assert(first->leaks.size() == second->leaks.size());

    int size = (first != nullptr) ? first->leaks.size() : second->leaks.size();
    std::set<Node*> all_leaks;
    if (first != nullptr)
        for (size_t i = 0; i < first->leaks.size(); ++i)
            all_leaks.insert(first->leaks[i].begin(), first->leaks[i].end());

    if (second != nullptr)
        for (size_t i = 0; i < second->leaks.size(); ++i)
            all_leaks.insert(second->leaks[i].begin(), second->leaks[i].end());

    LeakSet* ls = new LeakSet(size);
    std::fill(ls->leaks.begin(), ls->leaks.end(), all_leaks);
    return ls;
}

LeakSet* extract(LeakSet* ls, size_t begin, size_t end) {
    REMOVE_DISABLED
    if (ls == nullptr) return nullptr;
    assert(begin <= end && begin >= 0 && end <= ls->leaks.size());

    LeakSet* res = new LeakSet(std::vector<std::set<Node*>>(ls->leaks.cbegin() + begin, ls->leaks.cbegin() + end + 1));

    if (not is_ls_real(res)) return nullptr;
    return res;
}

LeakSet* sextend(LeakSet* ls, size_t new_size) {
    REMOVE_DISABLED
    if (ls == nullptr) return nullptr;
    if (new_size == ls->leaks.size()) return ls;
    assert(new_size > 0 && new_size >= ls->leaks.size());

    LeakSet* res = new LeakSet(new_size);
    std::copy(ls->leaks.cbegin(), ls->leaks.cend(), res->leaks.begin());

    // Fill remaining created bits to MSB ls
    for (size_t i = ls->leaks.size(); i < res->leaks.size(); ++i)
        res->leaks[i].insert(ls->leaks[ls->leaks.size()-1].begin(), ls->leaks[ls->leaks.size()-1].end());

    return res;
}

LeakSet* extend(LeakSet* ls, size_t new_size) {
    REMOVE_DISABLED
    if (ls == nullptr) return nullptr;
    if (new_size == ls->leaks.size()) return ls;
    assert(new_size > 0 && new_size >= ls->leaks.size());

    LeakSet* res = new LeakSet(new_size);
    std::copy(ls->leaks.cbegin(), ls->leaks.cend(), res->leaks.begin());

    return res;
}

LeakSet* rextend(LeakSet* ls, size_t new_size) {
    REMOVE_DISABLED
    if (ls == nullptr) return nullptr;
    if (new_size == ls->leaks.size()) return ls;
    assert(new_size > 0 && new_size >= ls->leaks.size());

    LeakSet* res = new LeakSet(new_size);
    std::copy(ls->leaks.cbegin(), ls->leaks.cend(), res->leaks.begin() + (new_size - ls->leaks.size()));

    return res;
}

LeakSet* blit(LeakSet* ls, LeakSet* source, size_t begin, size_t end, size_t size) {
    REMOVE_DISABLED
    if (ls == nullptr and source == nullptr) return nullptr;
    assert(begin <= end && begin >= 0 && size > 0);
    assert(ls == nullptr || (end < ls->leaks.size() && size == ls->leaks.size()));
    assert(source == nullptr || (end-begin+1) == source->leaks.size());
    assert((source == nullptr || ls == nullptr) || source->leaks.size() < ls->leaks.size());

    LeakSet* res = (ls != nullptr) ? new LeakSet(ls->leaks) : new LeakSet(size);
    source = (source != nullptr) ? source : new LeakSet(end - begin + 1);

    std::ranges::copy_n(source->leaks.cbegin(), (end - begin + 1), res->leaks.begin() + begin);

    if (not is_ls_real(res)) return nullptr;
    return res;
}

LeakSet* replicate(LeakSet* source, size_t size) {
    REMOVE_DISABLED
    if (source == nullptr) return nullptr;
    if (size == 1) return source;
    assert(size > 0 && source->leaks.size() == 1);

    LeakSet* ls = new LeakSet(size);
    std::fill(ls->leaks.begin(), ls->leaks.end(), source->leaks[0]);

    return ls;
}

LeakSet* reduce(LeakSet* source) {
    REMOVE_DISABLED
    if (source == nullptr) return nullptr;

    // Create a 1bit leakset and fill it with every set of the vector
    LeakSet* ls = new LeakSet(1);
    for (size_t i = 0; i < source->leaks.size(); i++)
        ls->leaks[0].insert(source->leaks[i].begin(), source->leaks[i].end());

    return ls;
}

LeakSet* reduce_and_merge(LeakSet* source1, LeakSet* source2) {
    REMOVE_DISABLED
    // If either is not set, return the simple reduce of the other
    // It will not be an issue if it is nullptr as well
    if (source1 == nullptr) return reduce(source2);
    if (source2 == nullptr) return reduce(source1);
    assert(source1->leaks.size() == source2->leaks.size());

    // Create a 1bit leakset and fill it with every set of the vector
    LeakSet* ls = new LeakSet(1);
    for (size_t i = 0; i < source1->leaks.size(); i++) {
        ls->leaks[0].insert(source1->leaks[i].begin(), source1->leaks[i].end());
        ls->leaks[0].insert(source2->leaks[i].begin(), source2->leaks[i].end());
    }

    return ls;
}

// TODO: i would like to test this loop
LeakSet* shift_left(LeakSet* source, LeakSet* amount_ls, size_t size) {
    REMOVE_DISABLED
    if (source == nullptr && amount_ls == nullptr) return nullptr;

    std::set<Node*> amount_leak = flatten(amount_ls);
    LeakSet* ls = new LeakSet(size);
    for (size_t i = 0; i < size; ++i) {
        if (source != nullptr)
            amount_leak.insert(source->leaks.at(i).begin(), source->leaks.at(i).end());

        // Leak all selector ls to each bit and if they exist, the cumulative
        // Leaksets of the source up to the current bit
        ls->leaks.at(i).insert(amount_leak.begin(), amount_leak.end());
    }
    return ls;
}

LeakSet* shift_right(LeakSet* source, LeakSet* amount_ls, size_t size) {
    REMOVE_DISABLED
    if (source == nullptr && amount_ls == nullptr) return nullptr;

    std::set<Node*> amount_leak = flatten(amount_ls);
    LeakSet* ls = new LeakSet(size);
    for (int i = size - 1; i >= 0; --i) {
        if (source != nullptr)
            amount_leak.insert(source->leaks.at(i).begin(), source->leaks.at(i).end());

        // Leak all selector ls to each bit and if they exist, the cumulative
        // Leaksets of the source upper bits down to the current bit
        ls->leaks.at(i).insert(amount_leak.begin(), amount_leak.end());
    }
    return ls;
}

// We create a leakset of the size of the node and split each bit inside it
LeakSet* reg_stabilize(Node* node) {
    REMOVE_DISABLED
    if (node == nullptr or node->nature == CONST) return nullptr;

    LeakSet* ls = new leaks::LeakSet(node->width);
    for(int i = 0; i < node->width; ++i) {
        Node* tbi = &simplify(Extract(i, i, *node));
        if (tbi->nature != CONST) {
            ls->leaks.at(i).insert(tbi);
        }
    }
    return ls;
}

// We create a leakset of the size of the node and split each bit inside it
// when the bit is stable, otherwise, we copy the other leakset bit
LeakSet* partial_stabilize(LeakSet* ls_in, Node* node, uint32_t* stability) {
    REMOVE_DISABLED
    //if (ls_in == nullptr or node == nullptr) return nullptr;
    //if (node == nullptr or node->nature == CONST) return nullptr;

    bool empty_ls = true;
    // Maybe we can initialize with a full copy then fix the stables, it may be faster
    LeakSet* ls = new leaks::LeakSet(node->width);
    for (int i = 0; i < node->width; ++i) {
        if (stability[i/32] & (1 << (i%32))) {
            Node* tbi = &simplify(Extract(i, i, *node));
            if (tbi->nature != CONST) {
                ls->leaks.at(i).insert(tbi);
                empty_ls = false;
            }
        } else if (ls_in != nullptr) {
            ls->leaks.at(i) = ls_in->leaks.at(i);
            empty_ls = false;
        }
    }
    if (empty_ls or not is_ls_real(ls)) return nullptr;
    return ls;
}

// If lss are disabled, it should always be nullptr anyways
std::set<Node*> flatten(LeakSet* source) {
    std::set<Node*> res;
    if (source == nullptr) return res;

    for (size_t i = 0; i < source->leaks.size(); ++i)
        res.insert(source->leaks[i].begin(), source->leaks[i].end());

    return res;
}

bool symb_verify_without_glitch_bit(Node* a, bool remove_false_negatives, Properties prop, int order, int outputs) {
    bool not_leaking = false;
    for (int i = 0; i < a->width; i++) {
        Node& n = simplify(Extract(i, i, *a));
        switch (prop) {
            case Properties::TPS:
                not_leaking = (remove_false_negatives) ? tpsNoFalsePositive(n, true) : tps(n, true);
                break;
            case Properties::NI:
                not_leaking = ni(n, order);
                break;
            case Properties::SNI:
                not_leaking = ni(n, order - outputs);
                break;
            default:
                assert("Property not handled yet");
        }
        if (not not_leaking)
            return not_leaking;
    }
    return true;
}

bool symb_verify_without_glitch(Node* a, bool remove_false_negatives, Properties prop, int order, int outputs) {
    switch (prop) {
        case Properties::TPS:
            return (remove_false_negatives) ? tpsNoFalsePositive(*a, true) : tps(*a, true);
            break;
        case Properties::NI:
            return ni(*a, order, true);
            break;
        case Properties::SNI:
            return ni(*a, order - outputs, true);
            break;
        default:
            assert("Property not handled yet");
    }

    return true;
}

bool symb_verify_with_glitch(const std::set<Node*>& set, bool remove_false_negatives, Properties prop, int order, int outputs) {
    if (set.size() <= 0) return true;

    std::vector<Node*> leakages(set.begin(), set.end());
    switch (prop) {
        case Properties::TPS:
            return (remove_false_negatives) ? tpsNoFalsePositive(leakages, true) : tps(leakages, true);
            break;
        case Properties::NI:
            return ni(leakages, order, true);
            break;
        case Properties::SNI:
            return ni(leakages, order - outputs, true);
            break;
        default:
            assert("Property not handled yet");
    }

    return true;
}

std::ostream& print_leakage(const LeakSet* ls, std::ostream& out) {
    if (ls == nullptr) {
        out << "Unused leakset" << std::endl;
        return out;
    }

    for(size_t idx = 0; const auto& bit_leak : ls->leaks) {
        if (bit_leak.size() == 0) {
            out << "Bit: " << idx << ", empty" << std::endl;
        } else {
            out << "Bit: " << idx << ", Number of expressions: " << bit_leak.size() << std::endl;
            for(const auto& leak : bit_leak) {
                out << leak->verbatimPrint() << std::endl;
            }
        }
        ++idx;
        out << "---" << std::endl;
    }
    return out;
}

bool is_ls_real(const LeakSet* ls) {
    if (ls == nullptr) return false;

    for(auto& bit_leak : ls->leaks) {
        for(const auto& leak : bit_leak) {
            if (leak->nature != CONST)
                return true;
        }
    }
    return false;
}

void clear() {
    // Remove all ls that are in keepMem from Mem
    for (auto& elem : LeakSet::ls_keep_mem_)
        LeakSet::ls_mem_.erase(elem);

    // Now delete all mem elems
    // TODO: Carefull here, for outputs / inputs we may have elements deleted more than once !!!
    for (auto ls : LeakSet::ls_mem_)
        delete ls;

    // Populate global mem with remaining ls then clear keep ls
    LeakSet::ls_mem_ = LeakSet::ls_keep_mem_;
    LeakSet::ls_keep_mem_.clear();
}

void keep(LeakSet* ls) {
    if (ls == nullptr) return;
    LeakSet::ls_keep_mem_.insert(ls);
}

} // leaks
