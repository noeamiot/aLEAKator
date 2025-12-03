#ifndef __LEAKS_H__
#define __LEAKS_H__

#include <cassert>
#include <memory>
#include <set>
#include "verif_msi_pp.hpp"
#include "concrev.hpp"
#include "SHA256.hpp"

// The interface does not change depending on whether leaksets are enabled or not
// This is mandatory otherwise the state between the .h and .cpp file could be inconsistent
// when partial recompilation is performed

namespace leaks {

enum Properties { TPS, SNI, NI };

struct LeakSet {
    static std::set<LeakSet*> ls_mem_;
    static std::set<LeakSet*> ls_keep_mem_;
    std::vector<std::set<Node*>> leaks;
    LeakSet(size_t size) : leaks(std::vector<std::set<Node*>>(size, std::set<Node*>())) {
        LeakSet::ls_mem_.insert(this);
    };
    LeakSet(const std::vector<std::set<Node*>>& leaks) : leaks(leaks) {

//        for (size_t i = 0; i < leaks.size(); ++i) {
//            std::vector<Node*> leakages(leaks[i].begin(), leaks[i].end());
//            assert(leakages.size() == 0 || tps(leakages, true));
//        }
        LeakSet::ls_mem_.insert(this);
    };
    LeakSet(const LeakSet& first, const LeakSet& second) : leaks(first.leaks) {
        assert(first.leaks.size() == second.leaks.size());

        for (size_t i = 0; i < second.leaks.size(); ++i)
            leaks[i].insert(second.leaks[i].begin(), second.leaks[i].end());

//        for (size_t i = 0; i < second.leaks.size(); ++i) {
//            std::vector<Node*> leakages(leaks[i].begin(), leaks[i].end());
//            assert(leakages.size() == 0 || tps(leakages, true));
//        }
        LeakSet::ls_mem_.insert(this);
    };
    std::vector<std::set<Node*>> sets() {
        return leaks;
    }

    friend auto operator<<(std::ostream& os, LeakSet const& m) -> std::ostream& {
        os << std::endl << "Leakages :" << std::endl;
        for(auto& bitLeak : m.leaks) {
            if (bitLeak.size() == 0) {
                os << "No leakage for this bit, ";
            } else {
                for(const auto& leak : bitLeak) {
                    os << simplify(*leak) << std::endl;
                }
            }
            os << std::endl << "Next bit :" << std::endl;
        }
        return os;
    }
};

LeakSet* merge(LeakSet* first, LeakSet* second);
LeakSet* merge(const std::vector<LeakSet*>& lss);
LeakSet* mix(LeakSet* first, LeakSet* second);
LeakSet* extract(LeakSet* ls, size_t begin, size_t end);
LeakSet* sextend(LeakSet* ls, size_t new_size);
LeakSet* extend(LeakSet* ls, size_t new_size);
LeakSet* rextend(LeakSet* ls, size_t new_size);
LeakSet* blit(LeakSet* ls, LeakSet* source, size_t begin, size_t end, size_t size);
LeakSet* replicate(LeakSet* source, size_t size);

LeakSet* reduce(LeakSet* source);
LeakSet* reduce_and_merge(LeakSet* source1, LeakSet* source2);

LeakSet* shift_left(LeakSet* source, LeakSet* amount_ls, size_t size);
LeakSet* shift_right(LeakSet* source, LeakSet* amount_ls, size_t size);

LeakSet* reg_stabilize(Node* node);
LeakSet* partial_stabilize(LeakSet* ls_in, Node* node, uint32_t* stability);
std::set<Node*> flatten(LeakSet* source);

bool symb_verify_without_glitch_bit(Node* a, bool remove_false_negatives, Properties prop, int order, int outputs);
bool symb_verify_without_glitch(Node* a, bool remove_false_negatives, Properties prop, int order, int outputs);
bool symb_verify_with_glitch(const std::set<Node*>& set, bool remove_false_negatives, Properties prop, int order, int outputs);
std::ostream& print_leakage(const LeakSet* ls, std::ostream& out = std::cout);
bool is_ls_real(const LeakSet* ls);
void clear();
void keep(LeakSet* ls);

} // leaks

#endif /* __LEAKS_H__ */
