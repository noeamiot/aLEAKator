#ifndef PROGRAM_SETUP_H
#define PROGRAM_SETUP_H

#include "verif_msi_pp.hpp"
#include <cxxrtl/cxxrtl.h>
#include "manager.h"

// Forward declare top module
namespace cxxrtl_design {
    struct p_top;
}

class Program {
    struct Symbol {
        unsigned int addr;
        unsigned int size;
    };
public:
    // Interface to run programs
    virtual void init(Manager& manager, cxxrtl_design::p_top& top) = 0;
    virtual void conclude(Manager& manager, cxxrtl_design::p_top& top) = 0;
    virtual void hook(Manager& manager, cxxrtl_design::p_top& top) = 0;
    virtual void load(cxxrtl_design::p_top& top) = 0;
    virtual void symbols() = 0;

    virtual uint32_t pc(cxxrtl_design::p_top& top) = 0;

    // Interface for program helpers
    virtual size_t get_position(const std::string& symbol) = 0;
    virtual size_t get_index(const std::string& memory, const std::string& symbol) = 0;
    virtual size_t get_size(const std::string& symbol) = 0;
    virtual cxxrtl::value<32> get_mask(const std::string& symbol) = 0;

    std::map<std::string, Symbol> symbols_;
    std::map<Node*, Node*> symbol_to_conc_;

    virtual ~Program() = default;
};

// Program creation is disabled as there are pure virtual methods
template<class Derived>
class ProgramInterface : public Program {
    public:
        // All derivative class must implement an iterable memory_mapping_ <std::string, uint>
        // They must also implement the following pc_implem function
        virtual uint32_t pc(cxxrtl_design::p_top& top) override final {
            return static_cast<Derived*>(this)->pc_implem(top);
        }

        // Program helpers interface
        virtual void init(Manager& manager, cxxrtl_design::p_top& top) override final {
            static_cast<Derived*>(this)->init_implem(manager, top);
        }
        virtual void conclude(Manager& manager, cxxrtl_design::p_top& top) override final {
            static_cast<Derived*>(this)->conclude_implem(manager, top);
        }
        virtual void hook(Manager& manager, cxxrtl_design::p_top& top) override final {
            static_cast<Derived*>(this)->hook_implem(manager, top);
        }
        virtual void load(cxxrtl_design::p_top& top) override final {
            static_cast<Derived*>(this)->load_implem(top);
        }
        virtual void symbols() override final {
            static_cast<Derived*>(this)->symbols_implem();
        }

        // CPU Specifics (for now specialized for 32 bits processors)
        virtual size_t get_position(const std::string& symbol) override final {
            assert(symbols_.contains(symbol) && "Symbol table does not contain wanted symbol.");

            // No need to substract section mapping boundary, it is at least aligned by 2**2
            return (symbols_.at(symbol).addr)%4;
        }

        virtual size_t get_index(const std::string& memory, const std::string& symbol) override final {
            assert(static_cast<Derived*>(this)->memory_mapping_.contains(memory) && "Unknown memory region.");
            assert(symbols_.contains(symbol) && "Symbol table does not contain wanted symbol.");

            return (symbols_.at(symbol).addr - static_cast<Derived*>(this)->memory_mapping_.at(memory))/4;
        }

        virtual size_t get_size(const std::string& symbol) override final {
            assert(symbols_.contains(symbol) && "Symbol table does not contain wanted symbol.");

            return symbols_.at(symbol).size;
        }

        virtual cxxrtl::value<32> get_mask(const std::string& symbol) override final {
            assert(symbols_.contains(symbol) && "Symbol table does not contain wanted symbol.");

            size_t position = this->get_position(symbol);

            // Cannot generate a mask for a symbol that goes over the current word boundary
            assert(symbols_.at(symbol).size + position <= 4);

            return cxxrtl::value<32>((0xFFFFFFFFu >> ((4 - symbols_[symbol].size) * 8)) << (position * 8));
        }

        virtual ~ProgramInterface() = default;

    // Disable constructor to prevent UB
    protected:
        ProgramInterface() = default;
};

#endif // PROGRAM_SETUP_H
