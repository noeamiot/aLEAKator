#include "configuration.h"
#include "cxxrtl/capi/cxxrtl_capi.h"
#include "cxxrtl/cxxrtl.h"
#include "lss.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <regex>
namespace fs = std::filesystem;

#include "verif_msi_pp.hpp"

#include "manager.h"

// Should not be used before being initialised correctly by manager
std::ofstream simulation_logger;

// Small helper method
int current_ram() {
    // Returns total virtual RAM used by process in MB
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmSize:", 7) == 0){
            // Parse line
            const char* p = line;
            while (*p <'0' || *p > '9') p++;
            line[strlen(line)-3] = '\0';
            result = atoi(p);
            break;
        }
    }
    fclose(file);
    return result/1000;
}

Manager::Manager (cxxrtl::module& top, Configuration config) : config_(config) {
    top.debug_info(&this->dbg_items_, nullptr, "");
    config_.dump();

    // If the simulation logger is in a failed state, it means that the ofstream
    // has been written to before initialisation of manager, fail here, use top level
    // properly by letting the manager call it.
    if (simulation_logger.fail())
        throw std::invalid_argument( "Simulation log handler has been used before being properly defined." );

    // The = operator is a move operator, this is valid
    simulation_logger = std::ofstream{config_.working_path_/"simulation.txt"};

    // Parse circuit and fil internal maps
    std::ofstream parse_log_file(config_.working_path_/"parsed_dependencies.txt");
    this->parse_circuit(parse_log_file);

    // Log wires for wich we consider that there is no inputs even though they are in debug_items
    // We remove inputs and memories from this list to reduce previsible noise
    std::ofstream independent_wires_file(config_.working_path_/"independent_wires.txt");
    for(const auto& [name, element] : dbg_items_.table) {
        if (!this->topology_.contains(name) and !(element.begin()->flags & CXXRTL_INPUT)
          and !(element.begin()->type == CXXRTL_ALIAS) and !(element.begin()->type == CXXRTL_MEMORY)
          and !(element.begin()->next == nullptr)) {
            independent_wires_file << name << " not found, size: " << element.size() << std::endl;
        } else if(element.size() > 1) {
            this->split_wires_.insert(name);
            parse_log_file << name << std::endl;
        }
    }
    independent_wires_file.close();
    parse_log_file.close();

    this->init_database();
}

bool Manager::step(cxxrtl::module& top) {
    simulation_logger << "-------" << std::endl << "Cycle: " << steps_ << std::endl;
    std::cout << "Looping simulation step " << steps_ << ", ls : " << leaks::LeakSet::ls_mem_.size() << ", nodes : " << Node::nodeNum << ", cacheSet : " << verified_TWG_ << std::endl;
    if (is_measuring())
        std::cout << "Measure_step: " << measure_cycle() << std::endl;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    bool converged = top.eval();
    std::cout << "Evaluating cycle took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() << "ms." << std::endl;
    if (is_measuring()) {
        if ((config_.ORDER_VERIF_ == 1 and not this->verify()) or
            (config_.ORDER_VERIF_ > 1 and not this->verify_higher_order())) {
            std::cout << "Leaks found in simulation step " << steps_ << std::endl;
            ++leaking_cycles_;
            if (config_.EXIT_AT_FIRST_LEAK_ or config_.EXIT_AT_FIRST_LEAKING_CYCLE_)
                return false;
        }
    }

    if (top.commit() && !converged) {
        std::cout << "Evaluating further would mean delta-cycle execution, bailing out." << std::endl;
        return false;
    }

    // Before next cycle, clean all leaksets that aren't used anymore
    this->clean(top);
    std::cout << "Evaluating and verifying cycle took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() << "ms." << std::endl;

    std::cout << "Time elapsed since start: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin_time_).count() << "ms." << std::endl;
    std::cout << "RAM consumption: " << current_ram() << "MB." << std::endl;

    if (steps_ >= config_.CYCLES_TO_VERIFY_) {
        std::cout << "Reached the end of cycles to verify, stopping gracefully." << config_.CYCLES_TO_VERIFY_ << " - " << steps_ << std::endl;
        return false;
    }

    ++steps_;
    return true;
}

void Manager::clean(cxxrtl::module& top) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (auto& cycle : database_) {
        for (const auto& [wire, entry] : cycle) {
            leaks::keep(entry.leakset_);
        }
    }
    if (config_.ORDER_VERIF_ > 1) {
        for (auto& cycle : database_ho_) {
            for (const auto& [wire, entry] : cycle) {
                leaks::keep(entry.leakset_);
            }
        }
    }
    std::cout << "Keeping database and memories leaksets took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() << "ms." << std::endl;

    // Also keep needed state elements
    top.symb_keep();
    leaks::clear();
    // We do not clear ir anymore because, the value from the previous cycle is used in buildDatabase
    //wires_requiring_verification.clear();
    std::cout << "Keeping database + state leaksets took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() << "ms." << std::endl;
}

// Iterate over the structure once and create the internal structure of the maps inside database
// Also, populate a set containing all the registers and outputs wires
void Manager::init_database() {
    // Clear just in case
    database_[0].clear();

    registers_and_outputs_.clear();

    // Iterate over the debug structure
    for(const auto& [name, element] : dbg_items_.table) {
        // Now pre splitted values and parted values are taken in account, the same way
        if (element.size() > 1) {
            bool contains_wire = false;
            bool contains_output = false;
            uint16_t total_size = 0;

            // The parts are in the order of the lsb_at collumn
            for(const auto& part: element) {
                if (part.leakref == nullptr || part.type == CXXRTL_ALIAS) continue;
                assert(part.type != CXXRTL_MEMORY);

                // Identify wires or partial wires
                if (part.type == CXXRTL_WIRE)
                    contains_wire = true;

                if (part.flags & CXXRTL_OUTPUT)
                    contains_output = true;

                total_size += part.width;
            }

            // Always add registers and outputs to glitch verification
            if (contains_wire || contains_output)
                registers_and_outputs_.insert(name);

            Entry entry{(contains_wire ? Entry::WIRE : Entry::VALUE), total_size, contains_output,
                nullptr, nullptr};
            database_[0][name] = entry;
        } else {
            const auto& part = *(element.begin());

            if (part.leakref == nullptr || part.type == CXXRTL_ALIAS) continue;
            // Ignore memories for normal database
            if (part.type == CXXRTL_MEMORY) continue;

            // Always add registers and outputs to glitch verification
            if (part.type == CXXRTL_WIRE || (part.flags & CXXRTL_OUTPUT))
                registers_and_outputs_.insert(name);

            Entry entry{(part.type == CXXRTL_WIRE ? Entry::WIRE : Entry::VALUE),
                static_cast<uint16_t>(part.width), static_cast<bool>(part.flags & CXXRTL_OUTPUT), nullptr, nullptr};
            database_[0][name] = entry;
        }
    }

    // Retrospectively split exception signals, only valid for word verif
    if (not config_.BIT_VERIF_) {
        for (const auto& [signal, width] : config_.EXCEPTIONS_WORD_VERIF_) {
            assert(database_[0].contains(signal)); // Before they were ignored but just in case
            bool is_reg_or_out = registers_and_outputs_.contains(signal);

            for (int i = 0; i < database_[0][signal].width_ / width; i++) {
                uint16_t part_width = ((i+1)*width < database_[0][signal].width_) ? width : (database_[0][signal].width_ - (i*width));

                Entry entry{database_[0][signal].type_, part_width, database_[0][signal].is_output_, nullptr, nullptr};
                database_[0][std::string("split_exception_") + signal + std::string("_") + std::to_string(i)] = entry;

                // If we were supposed to check the original signal, add the sub one to the list
                if (is_reg_or_out)
                    registers_and_outputs_.insert(std::string("split_exception_") + signal + std::string("_") + std::to_string(i));
            }
            database_[0].erase(signal);

            // All sub signals have been added, so remove the original signal from the list
            registers_and_outputs_.erase(signal);
        }
    }

    // Duplicate skeleton
    database_[1] = database_[0];
}

void Manager::build_database() {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    // For the current cycle, this will contain the wires that applied stability, the outputs and
    // memory elements. Reset it before adding the ones for the just simulated cycle
    inputs_of_stabilized_gate_.clear();

    // Advance databases cycles
    // Note that the structure of database is fixed, the memory one is the only one cleared
    database_[1] = database_[0];
    //database_memory_[1] = database_memory_[0];
    // TODO: Check if we are correct here
    database_memory_[0].clear();
    database_memory_[1].clear();

    for(const auto& [name, element] : dbg_items_.table) {
        // Exceptionally split wires are handled after
        if (not config_.BIT_VERIF_ and config_.EXCEPTIONS_WORD_VERIF_.contains(name)) continue;

        if (element.size() > 1) {
            std::vector<Node*> nodes_to_merge;
            std::vector<std::set<Node*>> lss_to_merge(database_[0][name].width_);
            bool applied_stability = false;
            bool is_leakset_empty = true;
            unsigned int current_position = 0;

            // The parts are in the order of the lsb_at collumn
            for(const auto& part : element) {
                if (part.leakref == nullptr || part.type == CXXRTL_ALIAS) continue;

                // Detect if there is at least one bit of stability set
                for (size_t i = 0; i < (part.width + 31)/32; i++) {
                    if (part.stab[i] != 0x0u) {
                        applied_stability = true;
                        break;
                    }
                }

                // We asserted at construction that it was not a memory so it is valid
                auto [node, ls] = part.leakref->leak_single();
                nodes_to_merge.push_back(node);
                if (ls != nullptr) {
                    is_leakset_empty = false;
                    std::ranges::copy_n(ls->leaks.cbegin(), part.width, lss_to_merge.begin() + current_position);
                }

                current_position += part.width;
            }

            Node* merged_nodes = &simplify(Concat(nodes_to_merge));
            //assert(merged_nodes->width == current_position);
            leaks::LeakSet* merged_ls = (is_leakset_empty) ? nullptr : new leaks::LeakSet(lss_to_merge);

            // Add inputs of a gate to verif if the gate applied stabilty
            if (applied_stability and config_.USE_STABILITY_) {
                inputs_of_stabilized_gate_.insert(topology_[name].cbegin(), topology_[name].cend());
            }

            database_[0][name].expr_ = merged_nodes;
            database_[0][name].leakset_ = merged_ls;
        } else {
            const auto& part = *(element.begin());
            bool applied_stability = false;
            if (part.leakref == nullptr || part.type == CXXRTL_ALIAS) continue;

            // Memory are handled in a separate map
            if (part.type == CXXRTL_MEMORY) {
                for (const auto& [index, prev, curr] : part.leakref->leak_mem()) {
                    std::string cell_name = std::string("mem_") + name + std::string("_") + std::to_string(index);
                    database_memory_[0][cell_name] = Entry{Entry::WIRE, 32, false, curr.first, curr.second};
                    database_memory_[1][cell_name] = Entry{Entry::WIRE, 32, false, prev.first, prev.second};
                }
                continue;
            }

            for (size_t i = 0; i < (part.width + 31)/32; i++) {
                if (part.stab[i] != 0x0u) {
                    applied_stability = true;
                    break;
                }
            }

            // Add inputs of a gate to verif if the gate applied stabilty
            if (applied_stability and config_.USE_STABILITY_) {
                inputs_of_stabilized_gate_.insert(topology_[name].cbegin(), topology_[name].cend());
            }

            // Special case for multiplexors, as dead branch must be verified if the selector applied stability (even if the selected entry is fully unstable)
            // If is sufficent to not check for this case in the splitwires part as selectors can only be one bit wide
            if (config_.USE_STABILITY_ and applied_stability and mux_structures_.contains(name)) {
                if (part.curr[0] == 0x0u) {
                    // Dead branch is the S=1 so port B
                    inputs_of_stabilized_gate_.insert(mux_structures_[name].second.cbegin(), mux_structures_[name].second.cend());
                } else {
                    // Dead branch is the S=0 so port A
                    inputs_of_stabilized_gate_.insert(mux_structures_[name].first.cbegin(), mux_structures_[name].first.cend());
                }
            }

            auto [node, ls] = part.leakref->leak_single();
            database_[0][name].expr_ = node;
            database_[0][name].leakset_ = ls;
        }
    }

    // Retrospectively split exception signals, only valid for word verif
    if (not config_.BIT_VERIF_) {
        for (const auto& [signal, width] : config_.EXCEPTIONS_WORD_VERIF_) {
            bool is_input_of_stab_gate = inputs_of_stabilized_gate_.contains(signal);

            auto [node_to_split, ls_to_split] = dbg_items_.table[signal].begin()->leakref->leak_single();
            for (int i = 0; i < node_to_split->width / width; i++) {
                int up_border = (((i+1)*width < node_to_split->width) ? (i+1)*width : node_to_split->width) - 1;
                Node* node = &simplify(Extract(up_border, i*width, *node_to_split));
                leaks::LeakSet* ls = leaks::extract(ls_to_split, i*width, up_border);

                std::string split_name = std::string("split_exception_") + signal + std::string("_") + std::to_string(i);
                database_[0][split_name].expr_ = node;
                database_[0][split_name].leakset_ = ls;

                // If this signal was added due to stability, add the split element
                if (is_input_of_stab_gate)
                    inputs_of_stabilized_gate_.insert(split_name);
            }

            // In case it was tagged for verif, remove it
            inputs_of_stabilized_gate_.erase(signal);
        }
    }

    std::cout << "Building database for cycle took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() << "ms." << std::endl;
}

bool Manager::verify() {
    if (config_.SKIP_VERIF_CYCLES_ > steps_) {
        std::cout << "Skipping cycle " << steps_ << "/" << config_.SKIP_VERIF_CYCLES_ << std::endl;
        return false;
    }

    // We will want to verify all the wires that applied stability previous cycle so keep them in memory
    std::set<std::string> wires_elected_glitches = inputs_of_stabilized_gate_;
    std::cout << "Wires added to verif due to stability at previous cycle: " << wires_elected_glitches.size() << std::endl;

    // Add splited wires to verification everytime as they may cause local transitions
    std::cout << "Splitted wires: " << split_wires_.size() << std::endl;

    // Build the database for the current cycle
    this->build_database();

    // When force verif all is set
    if (config_.FORCE_VERIFY_ALL_) {
        auto ks = std::views::keys(database_[0]);
        wires_elected_glitches.insert(ks.begin(), ks.end());
    }

    // Add splitted wires for word verif
    if (not config_.BIT_VERIF_)
        wires_elected_glitches.insert(split_wires_.cbegin(), split_wires_.cend());

    // Add registers and primary outputs
    wires_elected_glitches.insert(registers_and_outputs_.cbegin(), registers_and_outputs_.cend());

    // Add inputs of stabilized gates of current cycle (modified by build_database call
    wires_elected_glitches.insert(inputs_of_stabilized_gate_.cbegin(), inputs_of_stabilized_gate_.cend());

    std::cout << "Wires added to verif due to stability at previous cycle: " << inputs_of_stabilized_gate_.size() << std::endl;
    std::cout << "Wires added to verif in the end: " << wires_elected_glitches.size() << std::endl;
    std::cout << "SIZE OF DB ANYWAYS: " << database_[0].size() << std::endl;

    std::cout << "Starting verif for cycle " << steps_ << std::endl;

    std::set<std::string> vwog_leaking, twog_leaking, vwg_leaking, twg_leaking;

    // For vwog, twog and twg (without over-approx), iterate over all database
    if (config_.VERIF_VALUE_WO_GLITCHES_ or config_.VERIF_TRANSITION_WO_GLITCHES_ or
        (config_.VERIF_TRANSITION_W_GLITCHES_ and not config_.TRANSITION_W_GLITCHES_OVER_APPROX_)) {
        for (const auto& [name, entry] : database_[0]) {
            if (config_.VERIF_VALUE_WO_GLITCHES_ and not this->is_secure_vwog(entry.expr_, entry.is_output_ ? 1 : 0)) {
                vwog_leaking.insert(name);
                if (config_.EXIT_AT_FIRST_LEAK_)
                    break;
            }

            if (config_.VERIF_TRANSITION_WO_GLITCHES_ and not this->is_secure_twog(entry, database_[1][name])) {
                twog_leaking.insert(name);
                if (config_.EXIT_AT_FIRST_LEAK_)
                    break;
            }

            if ((config_.VERIF_TRANSITION_W_GLITCHES_ and not config_.TRANSITION_W_GLITCHES_OVER_APPROX_) and not this->is_secure_twg(entry, database_[1][name])) {
                twg_leaking.insert(name);
                if (config_.EXIT_AT_FIRST_LEAK_)
                    break;
            }
        }
    }
    // For vwg and twg (with over-approx), iterate over flagged wires only
    if (config_.VERIF_VALUE_W_GLITCHES_ or
        (config_.VERIF_TRANSITION_W_GLITCHES_ and config_.TRANSITION_W_GLITCHES_OVER_APPROX_)) {
        for (const auto& name : wires_elected_glitches) {
            if (config_.VERIF_VALUE_W_GLITCHES_ and not this->is_secure_vwg(database_[0][name].leakset_, database_[0][name].is_output_ ? 1 : 0)) {
                vwg_leaking.insert(name);
                if (config_.EXIT_AT_FIRST_LEAK_)
                    break;
            }

            if ((config_.VERIF_TRANSITION_W_GLITCHES_ and config_.TRANSITION_W_GLITCHES_OVER_APPROX_) and not this->is_secure_twg(database_[0][name], database_[1][name])) {
                twg_leaking.insert(name);
                if (config_.EXIT_AT_FIRST_LEAK_)
                    break;
            }
        }
    }

    // Verify memories
    for (const auto& [name, entry] : database_memory_[0]) {
        if (config_.VERIF_VALUE_WO_GLITCHES_ and not this->is_secure_vwog(entry.expr_, entry.is_output_ ? 1 : 0)) {
            vwog_leaking.insert(name);
            if (config_.EXIT_AT_FIRST_LEAK_)
                break;
        }

        if (config_.VERIF_TRANSITION_WO_GLITCHES_ and not this->is_secure_twog(entry, database_memory_[1][name])) {
            twog_leaking.insert(name);
            if (config_.EXIT_AT_FIRST_LEAK_)
                break;
        }

        if (config_.VERIF_VALUE_W_GLITCHES_ and not this->is_secure_vwg(entry.leakset_, entry.is_output_ ? 1 : 0)) {
            vwg_leaking.insert(name);
            if (config_.EXIT_AT_FIRST_LEAK_)
                break;
        }

        if (config_.VERIF_TRANSITION_W_GLITCHES_ and not this->is_secure_twg(entry, database_memory_[1][name])) {
            twg_leaking.insert(name);
            if (config_.EXIT_AT_FIRST_LEAK_)
                break;
        }
    }

    // Now plot all graphs
//    for (auto& leak : vwog_leaking)
//        std::cout << "Leak by value without glitch on : " << leak << std::endl;
//    for (auto& leak : twog_leaking)
//        std::cout << "Leak by transition without glitch on : " << leak << std::endl;
//    for (auto& leak : vwg_leaking)
//        std::cout << "Leak by value with glitches on : " << leak << std::endl;
//    for (auto& leak : twg_leaking)
//        std::cout << "Leak by transition with glitches on : " << leak << std::endl;
//
//    std::cout << "---" << std::endl;
//    std::cout << "Verified " << verified_VWOG_ << " nodes by value w/o glitches in total for now" << std::endl;
//    std::cout << "Skipped verification for " << total_VWOG_ << " nodes by value w/o glitches in total for now" << std::endl;
//    std::cout << "---" << std::endl;
//    std::cout << "Verified " << verified_TWOG_ << " nodes by transition w/o glitches in total for now" << std::endl;
//    std::cout << "Skipped verification for " << total_TWOG_ << " nodes by transition w/o glitches in total for now" << std::endl;
//    std::cout << "---" << std::endl;
//    std::cout << "Verified " << verified_VWG_ << " sets by value with glitches in total for now" << std::endl;
//    std::cout << "Skipped verification for " << total_VWG_ << " sets by value with glitches in total for now" << std::endl;
//    std::cout << "---" << std::endl;
//    std::cout << "Verified " << verified_TWG_ << " sets by transition with glitches in total for now" << std::endl;
//    std::cout << "Skipped verification for " << total_TWG_ << " sets by transition with glitches in total for now" << std::endl;
//    std::cout << "---" << std::endl;

    if (config_.TRACK_LEAKS_) {
        std::cout << "Tracking root leaks" << std::endl;
        std::set<std::string> roots;
        std::map<std::string, bool> cache;

        for (const auto& wire : vwog_leaking)
            track_parents(cache, roots, wire, 0);
        for (const auto& wire : vwg_leaking)
            track_parents(cache, roots, wire, 0);
        for (const auto& wire : twog_leaking)
            track_parents(cache, roots, wire, 0);
        for (const auto& wire : twg_leaking)
            track_parents(cache, roots, wire, 0);

        std::cout << "Ended tracking root leaks, found: " << roots.size() << std::endl;
        if (config_.DETAIL_LEAKS_INFORMATION_)
            detail_leaks_all(roots);
        else
            std::ranges::copy(roots, std::ostream_iterator<std::string>(std::cout, " "));
    } else if (config_.DETAIL_LEAKS_INFORMATION_) {
        detail_leaks_vwog(vwog_leaking);
        detail_leaks_twog(twog_leaking);
        detail_leaks_vwg(vwg_leaking);
        detail_leaks_twg(twg_leaking);
    }

    uint32_t leaks_nb = vwog_leaking.size() + twog_leaking.size() + vwg_leaking.size() + twg_leaking.size();
    leaks_per_cycles_[measure_cycle()] = leaks_nb;

    return (leaks_nb == 0);
}

bool Manager::verify_higher_order() {
    if (config_.SKIP_VERIF_CYCLES_ > steps_) {
        std::cout << "Skipping cycle " << steps_ << "/" << config_.SKIP_VERIF_CYCLES_ << std::endl;
        return false;
    }

    // Build the database for the current cycle
    this->build_database();
    // Store the new cycle in database (database[0] can still be refered to as the last computed cycle)
    this->database_ho_.push_back(this->database_[0]);

    // When higher order is considered verif by value (with and without glitch) make sense trivially
    // For spatial: all combinations (eventually at bit level) of n-uplets of the current cycle (DO WE CONSIDER TRANSITIONS ?)
    // For temporal: all combinations (eventually at bit level) of n-uplets of the wire for the past cycles (NO NEED FOR TRANSITIONS)
    std::cout << "SIZE OF DB ANYWAYS: " << database_[0].size() << std::endl;

    bool is_secure;
    if (config_.HIGHER_ORDER_TYPE_ == Configuration::SPATIAL) {
        is_secure = this->verify_higher_order_spatial();
    } else {
        is_secure = this->verify_higher_order_temporal();
    }

    return is_secure;
}

bool Manager::verify_higher_order_spatial() {
    std::set<std::string> vwog_leaking, vwg_leaking;
    // The way to generate the combinations is slightly different between BIT and WORD verif
    // For the former, we must iterate through the elements and generate as much combinations as
    // there are bits for this wire
    // For WORD verification, we simply generate the combinations without duplicates

    // Spatial works on the freshly computed cycle only
    auto ks = std::views::keys(database_[0]);
    std::vector<std::string> keys{ ks.begin(), ks.end() };

    if (config_.BIT_VERIF_) {
        std::vector<std::pair<std::string, unsigned int>> keys_with_dups;

        // if we are performing bit-level verification, we need to take into account
        // that bits of a wire must be verified against each other and all other wires
        // So we add a new dimension by considering all bits of wires independently
        for (const auto& [name, entry] : database_[0])
            for (int i = 0; i < entry.expr_->width; ++i)
                keys_with_dups.push_back({name, i});

        // With duplicatas (usefull for bit verif)
        int n = keys_with_dups.size();
        int r = config_.ORDER_VERIF_;
        std::cout << "Combination of " << r << " among " << n << std::endl;
        std::vector<bool> v(n);
        std::fill(v.end() - r, v.end(), true);
        std::vector<std::pair<Entry, unsigned int>> combination;
        combination.reserve(r);

        do {
            combination.clear();
            // Two different bits of the same output wire are considered two outputs in bit-verif
            int outputs = 0;
            for (int i = 0; i < n; ++i) {
                if (v[i]) {
                    //std::cout << keys_with_dups[i].first << "[" << keys_with_dups[i].second << "], ";
                    //std::cout << i << " ";
                    combination.push_back({database_[0][keys_with_dups[i].first], keys_with_dups[i].second});
                    if (database_[0][keys_with_dups[i].first].is_output_)
                        ++outputs;
                }
            }
            //std::cout << "\n";
            // Now verify the built combination while taking care of the bit to handle for each Entry
            if (config_.VERIF_VALUE_WO_GLITCHES_) {
                std::vector<Node*> accumulate_verif_nodes;
                for (const auto& entry: combination)
                    accumulate_verif_nodes.push_back(&simplify(Extract(entry.second, entry.second, *entry.first.expr_)));

                Node* to_verif = &Concat(accumulate_verif_nodes);

                // Here it is ok to use is_secure even in BIT mode only because in higher order case, the method 
                // does not verify by bit but by word (here only containing needed bits)
                if (not this->is_secure_vwog(to_verif, outputs)) {
                    vwog_leaking.insert("TODO");
                    if (config_.EXIT_AT_FIRST_LEAK_)
                        break;
                }
            }
            if (config_.VERIF_VALUE_W_GLITCHES_) {
                std::vector<leaks::LeakSet*> accumulate_lss;
                for (const auto& entry: combination)
                    accumulate_lss.push_back(leaks::extract(entry.first.leakset_, entry.second, entry.second));

                leaks::LeakSet* to_verif = leaks::merge(accumulate_lss);

                // Here it is ok to use is_secure even in BIT mode as the leaksets are merged (one line containing all)
                if (not this->is_secure_vwg(to_verif, outputs)) {
                    vwg_leaking.insert("TODO");
                    if (config_.EXIT_AT_FIRST_LEAK_)
                        break;
                }
            }
        } while (std::next_permutation(v.begin(), v.end()));
    } else {
        // We only need to match each wire against all other wires without dupllicata when verifying words
        int n = keys.size();
        int r = config_.ORDER_VERIF_;
        std::vector<bool> v(n);
        std::fill(v.end() - r, v.end(), true);
        std::vector<Entry> combination;
        combination.reserve(r);

        do {
            combination.clear();
            // Wires are not combined with themselves so counting the number of outputs is valid
            int outputs = 0;
            for (int i = 0; i < n; ++i) {
                if (v[i]) {
                    //std::cout << i << " ";
                    //std::cout << keys[i] << ", ";
                    combination.push_back(database_[0][keys[i]]);
                    if (database_[0][keys[i]].is_output_)
                        ++outputs;
                }
            }
            //std::cout << "\n";
            if (config_.VERIF_VALUE_WO_GLITCHES_) {
                std::vector<Node*> accumulate_verif_nodes;
                for (const auto& entry: combination)
                    accumulate_verif_nodes.push_back(entry.expr_);

                Node* to_verif = &Concat(accumulate_verif_nodes);

                if (not this->is_secure_vwog(to_verif, outputs)) {
                    vwog_leaking.insert("TODO");
                    if (config_.EXIT_AT_FIRST_LEAK_)
                        break;
                }
            }
            if (config_.VERIF_VALUE_W_GLITCHES_) {
                std::vector<leaks::LeakSet*> accumulate_lss;
                for (const auto& entry: combination)
                    accumulate_lss.push_back(entry.leakset_);

                leaks::LeakSet* to_verif = leaks::merge(accumulate_lss);

                if (not this->is_secure_vwg(to_verif, outputs)) {
                    vwg_leaking.insert("TODO");
                    if (config_.EXIT_AT_FIRST_LEAK_)
                        break;
                }
            }
        } while (std::next_permutation(v.begin(), v.end()));
    }
    return (vwog_leaking.size() + vwg_leaking.size() == 0);
}

bool Manager::verify_higher_order_temporal() {
    std::set<std::string> vwog_leaking, vwg_leaking;
    // We will want to verify all n-uplets (eventually in bits) in time involving a fixed wire w
    // As we verify cycle by cycle, there is no need to verify old combinations
    // Thus, generate combinations an order lower then add our current cycle

    // We do not consider the first cycle (it is not really higher order anyways)
    // And it is an issue for our combination computation (we cannot generate a combination on a
    // vector of size 0)
    if (this->measure_cycle() == 0)
        return true;

    // No output counting, this is not defined for SNI
    unsigned int n = this->measure_cycle();
    unsigned int r = (config_.ORDER_VERIF_ > n) ? n : config_.ORDER_VERIF_ - 1; // If order is bigger than our number of cycles, make a single combination of the current size
    std::vector<bool> v(n);
    std::fill(v.end() - r, v.end(), true);

    do {
        // To print cycles combination performed
        //for (unsigned int i = 0; i < n; ++i) {
        //    if (v[i])
        //        std::cout << i << " ";
        //std::cout << n << "\n";

        // Loop through all wires, note that the entry for database[0] is used (this is the n that is
        // always involed in verification)
        for (const auto& [name, entry] : database_[0]) {
            if (config_.VERIF_VALUE_WO_GLITCHES_) {
                if (config_.BIT_VERIF_) {
                    for (int bit = 0; bit < entry.expr_->width; ++bit) {
                        std::vector<Node*> accumulate_verif_nodes{entry.expr_};
                        for (unsigned int i = 0; i < n; ++i) {
                            if (v[i]) {
                                accumulate_verif_nodes.push_back(&simplify(Extract(bit, bit, *this->database_ho_.at(i).at(name).expr_)));
                            }
                        }

                        Node* to_verif = &Concat(accumulate_verif_nodes);
                        if (not this->is_secure_vwog(to_verif, 0)) {
                            vwog_leaking.insert("TODO");
                            // The goto is justified by the overhead of exiting deeply nested loops
                            // And is only used on quick exit
                            if (config_.EXIT_AT_FIRST_LEAK_)
                                goto exit_verify_ho_temporal;
                        }
                    }
                } else {
                    std::vector<Node*> accumulate_verif_nodes{entry.expr_};
                    for (unsigned int i = 0; i < n; ++i) {
                        if (v[i]) {
                            accumulate_verif_nodes.push_back(this->database_ho_.at(i).at(name).expr_);
                        }
                    }

                    Node* to_verif = &Concat(accumulate_verif_nodes);
                    if (not this->is_secure_vwog(to_verif, 0)) {
                        vwog_leaking.insert("TODO");
                        if (config_.EXIT_AT_FIRST_LEAK_)
                            goto exit_verify_ho_temporal;
                    }
                }
            }
            // This is valid for bit and word verif as we merge lss of same size
            if (config_.VERIF_VALUE_W_GLITCHES_) {
                std::vector<leaks::LeakSet*> accumulate_lss{entry.leakset_};
                for (unsigned int i = 0; i < n; ++i) {
                    if (v[i]) {
                        accumulate_lss.push_back(this->database_ho_.at(i).at(name).leakset_);
                    }
                }
                leaks::LeakSet* to_verif = leaks::merge(accumulate_lss);

                if (not this->is_secure_vwg(to_verif, 0)) {
                    vwg_leaking.insert("TODO");
                    if (config_.EXIT_AT_FIRST_LEAK_)
                        goto exit_verify_ho_temporal;
                }
            }
        }
    } while (std::next_permutation(v.begin(), v.end()));
    // All exit due to exit at first leak will end up here as well as notmal exit
exit_verify_ho_temporal:
    return (vwog_leaking.size() + vwg_leaking.size() == 0);
}

// This function is only called on leaking nodes, it is safe to assume that the currently queried
// wire is leaking
void Manager::track_parents(std::map<std::string, bool>& cache, std::set<std::string>& roots, const std::string& needle, int depth) {
    // Some terminal conditions, memories are roots if they leak and we do not explore already explored paths
    if (needle.starts_with("mem_")) {
        //std::cout << depth << ":Wire " << needle << " is memory, considered root leakage." << std::endl;
        roots.insert(needle);
        cache[needle] = true;
        return;
    }

    if (cache.contains(needle)) {
        //std::cout << depth << ": Evincing, in cache: " << needle << std::endl;
        return;
    }
    cache[needle] = true;

    if (not topology_.contains(needle)) {
        //std::cout << depth << ": " << needle << " is not in topo map" << std::endl;
        return;
    }

    if (database_[0].at(needle).type_ == Entry::WIRE) {
        //std::cout << depth << ": ROOT FOUND Wire " << needle << " is synchronous, considered root leakage." << std::endl;
        roots.insert(needle);
        return;
    }

    // Go through all the parents, if they leak inspect their parents and so on
    // If none leak, this is the root of the leak
    bool at_least_one_leaking_parent = false;
    std::set<std::string> parents = this->topology_[needle];
    //if (parents.empty())
    //    std::cout << depth << ":Wire " << needle << " has no parents." << std::endl;

    for (const auto& wire : parents) {
        bool is_leaking = false;
        if (cache.contains(wire)) {
            //std::cout << depth << ":Parent is in cache: " << wire << std::endl;
            is_leaking = cache[wire];
        } else {
            //std::cout << depth << ":Parent is not in cache: " << wire << std::endl;
            is_leaking |= (not is_leaking and config_.VERIF_TRANSITION_W_GLITCHES_ and not this->is_secure_twg(database_[0].at(wire), database_[1].at(wire)));
            is_leaking |= (not is_leaking and config_.VERIF_VALUE_W_GLITCHES_ and not this->is_secure_vwg(database_[0].at(wire).leakset_, database_[0].at(wire).is_output_ ? 1 : 0));
            is_leaking |= (not is_leaking and config_.VERIF_TRANSITION_WO_GLITCHES_ and not this->is_secure_twog(database_[0].at(wire), database_[1].at(wire)));
            is_leaking |= (not is_leaking and config_.VERIF_VALUE_WO_GLITCHES_ and not this->is_secure_vwog(database_[0].at(wire).expr_, database_[0].at(wire).is_output_ ? 1 : 0));

            // If the parent is leaking and not in cache, look for its root 
            if (is_leaking) {
                //std::cout << depth << ":Checking parent: " << wire << std::endl;
                track_parents(cache, roots, wire, depth+1);
            }
        }
        at_least_one_leaking_parent |= is_leaking;
    }
    if (not at_least_one_leaking_parent or parents.empty()) {
        //std::cout << depth << ": ROOT FOUND: " << needle << std::endl;
        roots.insert(needle);
    }

    return;
}

void Manager::detail_wire_info(const std::string& wire) {
    std::string handled_wire;
    int idx = 0;
    if (wire.starts_with("mem_") or wire.starts_with("split_exception_")) {
        std::cout << "Wire is either memory or a splited exception, extracting data from name." << std::endl;
        handled_wire = wire.substr(wire.find_first_of("_")+1, wire.find_last_of("_")-wire.find_first_of("_")-1);
        idx = std::stoi(wire.substr(wire.find_last_of("_")+1));
    } else {
        handled_wire = wire;
    }

    std::cout << "Handled wire name is: " << handled_wire << std::endl;

    if (split_wires_.contains(handled_wire))
        std::cout << "Wire is a splitted at least once." << std::endl;

    // TODO: Maybe do better
    if (not dbg_items_.table.contains(handled_wire)) {
        std::cout << "Wire is not in dbg_items, it may be due to it being splitted in synth." << std::endl;
        return;
    }

    // Memory or wire
    for (const auto& [type, data] : dbg_items_.at(handled_wire).at(0).attrs->map) {
        // Ignore the keep attribute, we set it in yosys for technical reasons that do not add
        // any insight here
        if (type == "keep")
            continue;

        std::cout << "Metadata: " << type << std::endl;
        std::cout << "Value: ";
        switch (data.value_type) {
            case cxxrtl::metadata::STRING:
                std::cout << data.as_string() << std::endl;
                break;
            case cxxrtl::metadata::UINT:
                std::cout << data.as_uint() << std::endl;
                break;
            case cxxrtl::metadata::SINT:
                std::cout << data.as_sint() << std::endl;
                break;
            case cxxrtl::metadata::DOUBLE:
                std::cout << data.as_sint() << std::endl;
                break;
            default:
                std::cout << "Could not determine type to display" << std::endl;
                break;
        }
    }

    //std::cout << "Verilog wire name: " << dbg_items_[wire].attrs << std::endl;
    if (dbg_items_.is_memory(handled_wire)) {
        std::cout << "Wire is a memory, leaking index is: " << idx << std::endl;
        std::cout << "Memory depth is: " << dbg_items_.at(handled_wire).at(0).depth << std::endl;
    } else if (config_.EXCEPTIONS_WORD_VERIF_.contains(handled_wire)) {
        std::cout << "Wire is a split exception, leaking index is: " << idx << std::endl;
    }

    uint32_t flags = dbg_items_.at(handled_wire).at(0).flags;
    if ((flags & CXXRTL_INPUT) or (flags & CXXRTL_INOUT))
        std::cout << "Wire is an input" << std::endl;
    if ((flags & CXXRTL_OUTPUT) or (flags & CXXRTL_INOUT))
        std::cout << "Wire is an output" << std::endl;

    if (flags & CXXRTL_DRIVEN_COMB)
        std::cout << "Wire is driven by a combinatorial gate" << std::endl;
    else if (flags & CXXRTL_DRIVEN_SYNC)
        std::cout << "Wire is driven by a synchronous gate" << std::endl;
    else
        std::cout << "Wire is not driven" << std::endl;

    // TODO: Not enough, will show one for splitted wires
    std::cout << "Width of the wire: " << dbg_items_.at(handled_wire).at(0).width << std::endl;
}

void Manager::detail_leaks_vwog(const std::set<std::string>& wires) {
    for (const auto& wire : wires) {
        std::cout << "Wire: " << wire << " is leaking in Value without glitches." << std::endl;
        detail_wire_info(wire);
        if (config_.DETAIL_SHOW_EXPRESSION_) {
            std::cout << "Its current expression is: " << database_[0][wire].expr_->verbatimPrint() << std::endl;
        }
    }
}

void Manager::detail_leaks_twog(const std::set<std::string>& wires) {
    for (const auto& wire : wires) {
        std::cout << "Wire: " << wire << " is leaking in Transition without glitches." << std::endl;
        detail_wire_info(wire);
        if (config_.DETAIL_SHOW_EXPRESSION_) {
            std::cout << "Its current expression is: " << database_[0][wire].expr_->verbatimPrint() << std::endl;
            std::cout << "Its previous expression is: " << database_[1][wire].expr_->verbatimPrint() << std::endl;
        }
    }
}

void Manager::detail_leaks_vwg(const std::set<std::string>& wires) {
    for (const auto& wire : wires) {
        detail_wire_info(wire);
        std::cout << "Wire: " << wire << " is leaking in Value with glitches." << std::endl;
        if (config_.DETAIL_SHOW_EXPRESSION_) {
            std::cout << "Its current leakset is: ";
            leaks::print_leakage(database_[0][wire].leakset_);
            std::cout << std::endl;
        }
    }
}

void Manager::detail_leaks_twg(const std::set<std::string>& wires) {
    for (const auto& wire : wires) {
        detail_wire_info(wire);
        std::cout << "Wire: " << wire << " is leaking in Transition with glitches." << std::endl;
        if (config_.DETAIL_SHOW_EXPRESSION_) {
            std::cout << "Its current leakset is: ";
            leaks::print_leakage(database_[0][wire].leakset_);
            std::cout << std::endl;
            std::cout << "Its previous leakset is: ";
            leaks::print_leakage(database_[1][wire].leakset_);
            std::cout << std::endl;
        }
    }
}

void Manager::detail_leaks_all(const std::set<std::string>& wires) {
    for (const auto& wire : wires) {
        detail_wire_info(wire);
        std::cout << "Wire: " << wire << "." << std::endl;
        if (config_.DETAIL_SHOW_EXPRESSION_) {
            // TODO: This does not handle splitted wires
            Entry& current = (database_[0].contains(wire)) ? database_[0][wire] : database_memory_[0][wire];
            Entry& previous = (database_[1].contains(wire)) ? database_[1][wire] : database_memory_[1][wire];
            std::cout << "Its current expression is: " << current.expr_->verbatimPrint() << std::endl;
            std::cout << "Its current leakset is: ";
            leaks::print_leakage(current.leakset_);
            std::cout << std::endl;
            std::cout << "Its previous expression is: " << previous.expr_->verbatimPrint() << std::endl;
            std::cout << "Its previous leakset is: ";
            leaks::print_leakage(previous.leakset_);
            std::cout << std::endl;
        }
    }
}

bool Manager::is_secure_vwog(Node* node, int outputs) {
    ++total_VWOG_;

    if (cache_.is_node_trivial(node)) {
        cache_.incr_trivial_nodes();
        return true;
    }
    if (Cache::CacheVerdict verdict = cache_.is_cached_node_secure(node); verdict.in_cache_)
        return verdict.is_secure_;

    ++verified_VWOG_;

    bool verification_verdict;
    if (config_.BIT_VERIF_ and config_.ORDER_VERIF_ == 1)
        verification_verdict = leaks::symb_verify_without_glitch_bit(node, config_.REMOVE_FALSE_NEGATIVE_, config_.SECURITY_PROPERTY_, config_.ORDER_VERIF_, outputs);
    else
        verification_verdict = leaks::symb_verify_without_glitch(node, config_.REMOVE_FALSE_NEGATIVE_, config_.SECURITY_PROPERTY_, config_.ORDER_VERIF_, outputs);

    // No cache for SNI, it depends on the maxShareOcc would need another kind of cache
    if (config_.SECURITY_PROPERTY_ != leaks::Properties::SNI)
        cache_.add_node_to_cache(node, verification_verdict);
    return verification_verdict;
}

bool Manager::is_secure_twog(const Entry& entry_curr, const Entry& entry_prev) {
    ++total_TWOG_;

    Node* current_node = entry_curr.expr_;
    Node* previous_node = entry_prev.expr_;

    // Transitions are not verified at first cycle
    if (previous_node == nullptr) return true;
    if (current_node->nature == CONST and previous_node->nature == CONST) {
        cache_.incr_trivial_nodes();
        return true;
    }

    // If node is const, verify node before or the other way around if neither is const verify both
    Node* node;
    if (current_node->nature == CONST)
        node = previous_node;
    else if (previous_node->nature == CONST)
        node = current_node;
    else
        node = &simplify(*current_node ^ *previous_node);

    if (cache_.is_node_trivial(node)) {
        cache_.incr_trivial_nodes();
        return true;
    }
    if (Cache::CacheVerdict verdict = cache_.is_cached_node_secure(node); verdict.in_cache_)
        return verdict.is_secure_;

    ++verified_TWOG_;

    // No output counting, transition is not defined for SNI
    bool verification_verdict;
    if (config_.BIT_VERIF_)
        verification_verdict = leaks::symb_verify_without_glitch_bit(node, config_.REMOVE_FALSE_NEGATIVE_, config_.SECURITY_PROPERTY_, config_.ORDER_VERIF_, 0);
    else
        verification_verdict = leaks::symb_verify_without_glitch(node, config_.REMOVE_FALSE_NEGATIVE_, config_.SECURITY_PROPERTY_, config_.ORDER_VERIF_, 0);

    // No SNI with transitions, can always add to cache
    cache_.add_node_to_cache(node, verification_verdict);
    return verification_verdict;
}

bool Manager::is_secure_vwg(leaks::LeakSet* leakset, int outputs) {
    ++total_VWG_;

    if (leakset == nullptr) {
        cache_.incr_trivial_sets();
        return true;
    }

    if (config_.BIT_VERIF_) {
        for (auto& set : leakset->sets()) {
            if (cache_.is_set_trivial(set)) {
                cache_.incr_trivial_sets();
                continue;
            }
            if (Cache::CacheVerdict verdict = cache_.is_cached_set_secure(set); verdict.in_cache_) {
                if (verdict.is_secure_)
                    continue;
                return false;
            }

            ++verified_VWG_;

            // If this bit is secure, continue to next bit. Otherwise stop there as non-secure
            bool verification_verdict = leaks::symb_verify_with_glitch(set, config_.REMOVE_FALSE_NEGATIVE_, config_.SECURITY_PROPERTY_, config_.ORDER_VERIF_, outputs);
            // No cache for SNI, it depends on the maxShareOcc would need another kind of cache
            if (config_.SECURITY_PROPERTY_ != leaks::Properties::SNI)
                cache_.add_set_to_cache(set, verification_verdict);
            if (verification_verdict)
                continue;
            return false;
        }
        // If all bits were secure
        return true;
    } else {
        std::set<Node*> set = leaks::flatten(leakset);
        if (cache_.is_set_trivial(set)) {
            cache_.incr_trivial_sets();
            return true;
        }
        if (Cache::CacheVerdict verdict = cache_.is_cached_set_secure(set); verdict.in_cache_)
            return verdict.is_secure_;

        ++verified_VWG_;

        bool verification_verdict = leaks::symb_verify_with_glitch(set, config_.REMOVE_FALSE_NEGATIVE_, config_.SECURITY_PROPERTY_, config_.ORDER_VERIF_, outputs);
        if (config_.SECURITY_PROPERTY_ != leaks::Properties::SNI)
            cache_.add_set_to_cache(set, verification_verdict);
        return verification_verdict;
    }
}

bool Manager::is_secure_twg(const Entry& entry_curr, const Entry& entry_prev) {
    leaks::LeakSet* current_leakset = entry_curr.leakset_;

    // Without over-approx the stable node of previous cycle is used
    // Otherwise, the previous leakset is used
    leaks::LeakSet* previous_leakset;
    if (config_.TRANSITION_W_GLITCHES_OVER_APPROX_)
        previous_leakset = entry_prev.leakset_;
    else
        previous_leakset = leaks::reg_stabilize(entry_prev.expr_);

    ++total_TWG_;

    if (current_leakset == nullptr and previous_leakset == nullptr) {
        cache_.incr_trivial_sets();
        return true;
    }

    // No output counting, transition is not defined for SNI
    if (config_.BIT_VERIF_) {
        leaks::LeakSet* leakset = leaks::merge(current_leakset, previous_leakset);
        for (auto& set : leakset->sets()) {
            if (cache_.is_set_trivial(set)) {
                cache_.incr_trivial_sets();
                continue;
            }
            if (Cache::CacheVerdict verdict = cache_.is_cached_set_secure(set); verdict.in_cache_) {
                if (verdict.is_secure_)
                    continue;
                return false;
            }
            ++verified_TWG_;

            bool verification_verdict = leaks::symb_verify_with_glitch(set, config_.REMOVE_FALSE_NEGATIVE_, config_.SECURITY_PROPERTY_, config_.ORDER_VERIF_, 0);
            // No transitions with SNI so it's ok here
            cache_.add_set_to_cache(set, verification_verdict);
            if (verification_verdict)
                continue;
            return false;
        }
        // If all bits were secure
        return true;
    } else {
        // Merge all Nodes of both sets inside a set to verify
        std::set<Node*> set = leaks::flatten(current_leakset);
        set.merge(leaks::flatten(previous_leakset));

        if (cache_.is_set_trivial(set)) {
            cache_.incr_trivial_sets();
            return true;
        }
        if (Cache::CacheVerdict verdict = cache_.is_cached_set_secure(set); verdict.in_cache_)
            return verdict.is_secure_;
        ++verified_TWG_;

        bool verification_verdict = leaks::symb_verify_with_glitch(set, config_.REMOVE_FALSE_NEGATIVE_, config_.SECURITY_PROPERTY_, config_.ORDER_VERIF_, 0);
        // No transitions with SNI so it's ok here
        cache_.add_set_to_cache(set, verification_verdict);
        return verification_verdict;
    }
}

void Manager::begin_measure() {
    if (measure_started_) {
        std::cout << "Measure already started." << std::endl;
        return;
    }
    begin_cycle_ = steps_;
    begin_measure_time_ = std::chrono::steady_clock::now();
    measure_started_ = true;
}

void Manager::end_measure() {
    if (not measure_started_ or measure_ended_) {
        std::cout << "Measure not started or already ended." << std::endl;
        return;
    }
    measure_ended_ = true;
    end_cycle_ = steps_;
    end_measure_time_ = std::chrono::steady_clock::now();
    end_ram_ = current_ram();
}

void Manager::stat() {
    if (not measure_started_ or not measure_ended_) {
        std::cout << "No stats to show, something went wrong with capture" << std::endl;
        return;
    }

    std::cout << "Statistics: " << std::endl;
    std::cout << "Cycles: " << end_cycle_ - begin_cycle_ << std::endl;
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_measure_time_ - begin_measure_time_).count() << "ms." << std::endl;
    std::cout << "RAM: " << end_ram_ << "MB." << std::endl;
    std::cout << "VerifSets: " << verified_TWG_ + verified_VWG_ << std::endl;
    std::cout << "VerifNodes: " << verified_TWOG_ + verified_VWOG_ << std::endl;
    std::cout << "LeakingCycles: " << leaking_cycles_ << std::endl;

    std::cout << "For this cycle:" << std::endl;
    std::cout << "Number of trivial sets verifications: " << cache_.get_trivial_sets() << std::endl;
    std::cout << "Number of setCacheHits : " << cache_.get_hits_sets() << std::endl;
    std::cout << "Number of trivial nodes verifications: " << cache_.get_trivial_nodes() << std::endl;
    std::cout << "Number of nodeCacheHits : " << cache_.get_hits_nodes() << std::endl;

    std::cout << "Number of leaks for each cycle: " << std::endl;
    for (auto const& [cycle, leaks] : leaks_per_cycles_) {
        //TODO: Temp: if (leaks == 0) continue;
        std::cout << "Cycle: " << cycle << ", Leaks: " << leaks << std::endl;
    }
}

void Manager::parse_circuit(std::ofstream& log) {
    fs::path path = config_.working_path_/"../.."/(config_.program_ + ".tsv");
    if (not fs::exists(path))
        throw std::invalid_argument( "Circuit topology file (tsv) not found. Looked for " + std::string(path) );

    // For each gate, get list of outputs
    std::map<std::string, std::set<std::string>> outputs;
    std::map<std::string, std::set<std::string>> inputs;
    std::map<std::string, std::string> muxGate;
    std::map<std::string, std::set<std::string>> muxAInputs;
    std::map<std::string, std::set<std::string>> muxBInputs;

    std::basic_regex regLine("\t");
    // We volontarly do not accept @ and % symbols in order to fuse all splitted wires
    // (their format is name@bit@ or name%bit%)
    std::basic_regex regWires("\\\\[a-zA-Z0-9\\[\\]':._\\$\\/]+");
    std::basic_regex regDots("\\.");

    log << "Following log are extracted lines, final dependencies are at the end of this file." << std::endl;

    std::ifstream ifile(path);
    std::string stringLine;
    while (std::getline(ifile, stringLine)) {
        // Split each lines and check that we got enough elements
        std::regex_token_iterator<std::string::iterator> i(stringLine.begin(), stringLine.end(), regLine, -1);
        std::regex_token_iterator<std::string::iterator> end;
        std::vector<std::string> line;
        std::copy(i, end, std::back_inserter(line));
        assert(std::distance(i, end)==6);

        log << line[1] << "-" << line[4] << "-" << line[5] << std::endl;

        // Last element is a wire or a list of wires with a specific format
        std::set<std::string> wires;

        std::string logl = line[5];
        for (std::smatch sm; regex_search(logl, sm, regWires);) {
            // Remove prefix \ and replace dots by space
            std::string filtered = sm.str().erase(0, 1);
            filtered = std::regex_replace(filtered, regDots, " ");

            wires.insert(filtered);
            logl = sm.suffix();

            if (logl.size() >= 2 and logl[1] == '[')
                split_wires_.insert(filtered);
        }

        // Exception for multiplexors on which we want to separate paths
        if (line[2] == "$mux" and line[4] == "in") {
            if (line[3] == "S") {
                assert(not muxGate.contains(line[1]) && "Two selectors for the same mux");
                assert(wires.size() == 1 && "Incorrect selector input wires");
                muxGate[line[1]] = *(wires.begin());
            } else if (line[3] == "A") {
                assert(not muxAInputs.contains(line[1]) && "Incorrect mux A port");
                muxAInputs[line[1]] = wires;
            } else if (line[3] == "B") {
                assert(not muxBInputs.contains(line[1]) && "Incorrect mux B port");
                muxBInputs[line[1]] = wires;
            }
        }

        // Then build the data structures
        if (line[4] == "in") {
            if (inputs.contains(line[1]))
                inputs[line[1]].insert(wires.begin(), wires.end());
            else
                inputs[line[1]] = wires;
        } else if (line[4] == "out") {
            if (outputs.contains(line[1]))
                outputs[line[1]].insert(wires.begin(), wires.end());
            else
                outputs[line[1]] = wires;
        } else if (line[4] != "pi" and line[4] != "po" and line[4] != "pio") {
            // We could, retreive primary in/outs if we wanted
            assert(false && "incorrect direction value");
        }
    }

    // Finally merge the data structures
    for (auto const& [gate, owires] : outputs) {
        for (auto const& owire : owires) {
            std::set<std::string> iwires = inputs.contains(gate) ? inputs[gate] : std::set<std::string>();
            topology_[owire] = iwires;
        }
    }

    // Merge the mux gates
    for (auto const& [gate, selector] : muxGate) {
        std::set<std::string> a = muxAInputs.contains(gate) ? muxAInputs[gate] : std::set<std::string>();
        std::set<std::string> b = muxBInputs.contains(gate) ? muxBInputs[gate] : std::set<std::string>();
        if (mux_structures_.contains(selector)) {
            mux_structures_[selector].first.insert(a.begin(), a.end());
            mux_structures_[selector].second.insert(b.begin(), b.end());
        } else {
            mux_structures_[selector] = std::pair<std::set<std::string>, std::set<std::string>>(a, b);
        }
    }

    log << "Merged data structures, dependecies are the following: " << std::endl;

    // Print for debug
    for (auto& [wire, iwires] : topology_) {
        log << wire << ": ";
        std::copy(std::begin(iwires), std::end(iwires), std::ostream_iterator<std::string>(log, ", "));
        log << std::endl;
    }

    log << "Extracted mux structures: " << std::endl;
    for (auto& [sel, pair] : mux_structures_) {
        log << "Sel: " << sel << std::endl;
        log << "A: ";
        std::copy(std::begin(pair.first), std::end(pair.first), std::ostream_iterator<std::string>(log, ", "));
        log << std::endl << "B: ";
        std::copy(std::begin(pair.second), std::end(pair.second), std::ostream_iterator<std::string>(log, ", "));
        log << std::endl;
    }

    log << "Splitted wires: " << std::endl;
    for (auto& w : split_wires_)
        log << w << std::endl;
}
