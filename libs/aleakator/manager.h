#ifndef TMPMANAGER_H
#define TMPMANAGER_H

#include <cstdint>
#include <cxxrtl/cxxrtl.h>

#include <sys/types.h>
#include "configuration.h"

#include "lss.h"

#include "utils.hpp"
struct Entry {
    enum ElementType : uint8_t {
        VALUE = CXXRTL_VALUE,
        WIRE  = CXXRTL_WIRE,
    };

    ElementType type_;
    uint16_t width_;
    bool is_output_;
    Node* expr_;
    leaks::LeakSet* leakset_;
};

// Simple helper class for cache that should be inlined
class Cache {
    private:
        std::map<Node*, bool> verified_nodes_{};
        std::map<std::set<Node*>, bool> verified_sets_{};

        unsigned int cache_hit_node_ = 0;
        unsigned int cache_hit_set_ = 0;

        unsigned int trivial_nodes_skipped_ = 0;
        unsigned int trivial_sets_skipped_ = 0;

    public:
        struct CacheVerdict {
            bool in_cache_ = false;
            bool is_secure_ = true;
        };
        bool is_node_trivial(Node* node) const {
            return (node->nature == CONST);
        }
        CacheVerdict is_cached_node_secure(Node* node) {
            // If following is true, it is in cache
            if (const auto& search = verified_nodes_.find(node); search != verified_nodes_.end()) {
                ++cache_hit_node_;
                return {true, search->second};
            } else {
                return {false, true};
            }
        }

        bool is_set_trivial(const std::set<Node*>& set) const {
            return (set.size() == 0);
            // Maybe add later the if only containing const but check that it is pertinent
        }
        CacheVerdict is_cached_set_secure(const std::set<Node*>& set) {
            // If following is true, it is in cache
            if (const auto& search = verified_sets_.find(set); search != verified_sets_.end()) {
                ++cache_hit_set_;
                return {true, search->second};
            } else {
                return {false, true};
            }
        }

        void incr_trivial_sets() { ++trivial_sets_skipped_; }
        void incr_trivial_nodes() { ++trivial_nodes_skipped_; }
        unsigned int get_trivial_sets() const { return trivial_sets_skipped_; }
        unsigned int get_hits_sets() const { return cache_hit_set_; }
        unsigned int get_trivial_nodes() const { return trivial_nodes_skipped_; }
        unsigned int get_hits_nodes() const { return cache_hit_node_; }

        void add_node_to_cache(Node* node, bool is_secure) {
            verified_nodes_[node] = is_secure;
        }
        void add_set_to_cache(const std::set<Node*>& set, bool is_secure) {
            verified_sets_[set] = is_secure;
        }
};

// The manager does not keep a reference to the top module as its type is not known at compilation
// time. The top module changes for each simulated circuit, but all top modules inherit from the
// pure virtual module class. Which we exploit to query module
class Manager {
    public:
        const Configuration config_;
        const std::chrono::steady_clock::time_point begin_time_ = std::chrono::steady_clock::now();

    private:
        cxxrtl::debug_items dbg_items_;

        std::map<std::string, std::set<std::string>> topology_;
        std::map<std::string, std::pair<std::set<std::string>, std::set<std::string>>> mux_structures_;
        std::set<std::string> split_wires_;
        std::set<std::string> registers_and_outputs_;
        std::set<std::string> inputs_of_stabilized_gate_;

        // index 0 is always the most recent cycle
        std::array<std::map<std::string, Entry>, 2> database_ = {};
        std::array<std::map<std::string, Entry>, 2> database_memory_ = {};
        // Only used for higher order
        std::vector<std::map<std::string, Entry>> database_ho_ = {};

        Cache cache_{};

        unsigned int steps_ = 0;

        // Statistics
        unsigned int total_VWOG_ = 0;
        unsigned int total_TWOG_ = 0;
        unsigned int total_VWG_ = 0;
        unsigned int total_TWG_ = 0;

        unsigned int verified_VWOG_ = 0;
        unsigned int verified_TWOG_ = 0;
        unsigned int verified_VWG_ = 0;
        unsigned int verified_TWG_ = 0;

        unsigned int leaking_cycles_ = 0;
        std::map<unsigned int, unsigned int> leaks_per_cycles_{};


        // Perfs statistics
        std::chrono::steady_clock::time_point begin_measure_time_;
        std::chrono::steady_clock::time_point end_measure_time_;
        bool measure_started_ = false;
        bool measure_ended_ = false;
        unsigned int begin_cycle_ = 0;
        unsigned int end_cycle_ = 0;
        // No begin_ram_, we cannot substract it meaningfully
        unsigned int end_ram_;

    public:
        Manager (cxxrtl::module& top, Configuration config);
        bool step(cxxrtl::module& top);
        unsigned int get_steps() const { return steps_; }
        std::vector<Node *> get_shares(Node& node, int nb_shares) const {
            if (config_.SECURITY_PROPERTY_ == leaks::TPS)
                return getPseudoShares(node, nb_shares);
            else
                return getRealShares(node, nb_shares);
        }

        void begin_measure();
        void end_measure();
        bool is_measuring() { return measure_started_ and not measure_ended_; }
        uint32_t measure_cycle() { return steps_ - begin_cycle_; }
        // TODO: Should take a stream and print to it
        void stat();

    private:
        void clean(cxxrtl::module& top);
        void parse_circuit(std::ofstream& log);

        void init_database();
        void build_database();

        bool verify();
        bool verify_higher_order();
        bool verify_higher_order_spatial();
        bool verify_higher_order_temporal();

        void track_parents(std::map<std::string, bool>& cache, std::set<std::string>& roots, const std::string& needle, int depth);

        void detail_wire_info(const std::string& wire);
        void detail_leaks_vwog(const std::set<std::string>& wires);
        void detail_leaks_twog(const std::set<std::string>& wires);
        void detail_leaks_vwg(const std::set<std::string>& wires);
        void detail_leaks_twg(const std::set<std::string>& wires);
        void detail_leaks_all(const std::set<std::string>& wires);

        bool is_secure_vwog(Node* expr, int outputs);
        bool is_secure_twog(const Entry& entry_curr, const Entry& entry_prev);
        bool is_secure_vwg(leaks::LeakSet* ls, int outputs);
        bool is_secure_twg(const Entry& entry_curr, const Entry& entry_prev);
};

#endif // MANAGER_H
