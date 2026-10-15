#ifndef PROGRAM_SETUP_H
#define PROGRAM_SETUP_H

#include "verif_msi_pp.hpp"
#include <stdexcept>
#include <format>
#include <cxxrtl/cxxrtl.h>
#include "manager.h"

// Forward declare top module
namespace cxxrtl_design {
    struct p_top;
}

class Program {
public:
    // Interface to run programs
    virtual void initial_init(Manager& manager) = 0;
    virtual void init(Manager& manager, cxxrtl_design::p_top& top) = 0;
    virtual void conclude(Manager& manager, cxxrtl_design::p_top& top) = 0;
    virtual void hook(Manager& manager, cxxrtl_design::p_top& top) = 0;

    virtual void load(Manager& manager, cxxrtl_design::p_top& top) = 0;
    virtual uint32_t pc(cxxrtl_design::p_top& top) = 0;

    // Interface for program helpers
    virtual size_t get_position(const std::string& symbol) = 0;
    virtual size_t get_index(const std::string& memory, const std::string& symbol) = 0;
    virtual size_t get_size(const std::string& symbol) = 0;
    virtual cxxrtl::value<32> get_mask(const std::string& symbol) = 0;
    virtual void assert_size_align(const std::string& symbol, uint32_t size, uint32_t alignment) = 0;

    // Interface to rationalise memory access accross CPUs (it not mendatory to implement them)
    static cxxrtl::memory<32>& ram(cxxrtl_design::p_top& top);
    static cxxrtl::memory<32>& rom(cxxrtl_design::p_top& top);

    RawProgram program_;
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
        virtual void initial_init(Manager& manager) override final {
            static_cast<Derived*>(this)->initial_init_implem(manager);
        }
        virtual void init(Manager& manager, cxxrtl_design::p_top& top) override final {
            static_cast<Derived*>(this)->init_implem(manager, top);
        }
        virtual void conclude(Manager& manager, cxxrtl_design::p_top& top) override final {
            static_cast<Derived*>(this)->conclude_implem(manager, top);
        }
        virtual void hook(Manager& manager, cxxrtl_design::p_top& top) override final {
            static_cast<Derived*>(this)->hook_implem(manager, top);
        }
        virtual void load(Manager& manager, cxxrtl_design::p_top& top) override final {
            static_cast<Derived*>(this)->load_implem(manager, top);
        }

        // CPU Specifics (for now specialized for 32 bits processors)
        virtual size_t get_position(const std::string& symbol) override final {
            if (not program_.symbols_.contains(symbol))
                throw std::out_of_range(
                    std::format(
                        "get_position: Symbol table does not contain symbol {}.",
                        symbol
                    )
                );

            // No need to substract section mapping boundary, it is at least aligned by 2**2
            return (program_.symbols_.at(symbol).addr_)%4;
        }

        virtual size_t get_index(const std::string& memory, const std::string& symbol) override final {
            if (not static_cast<Derived*>(this)->memory_mapping_.contains(memory))
                throw std::out_of_range(
                    std::format(
                        "get_index: Memory mapping does not contain region {}.",
                        memory
                    )
                );
            if (not program_.symbols_.contains(symbol))
                throw std::out_of_range(
                    std::format(
                        "get_index: Symbol table does not contain symbol {}.",
                        symbol
                    )
                );

            // As we do not have memory mapping max size, we can only check for underflow
            if (program_.symbols_.at(symbol).addr_ < static_cast<Derived*>(this)->memory_mapping_.at(memory))
                throw std::out_of_range(
                    std::format(
                        "get_index: Symbol {} is not in memory {}.",
                        symbol,
                        memory
                    )
                );

            return (program_.symbols_.at(symbol).addr_ - static_cast<Derived*>(this)->memory_mapping_.at(memory))/4;
        }

        virtual size_t get_size(const std::string& symbol) override final {
            if (not program_.symbols_.contains(symbol))
                throw std::out_of_range("get_size: Symbol table does not contain symbol '" + symbol + "'.");

            return program_.symbols_.at(symbol).size_;
        }

        virtual cxxrtl::value<32> get_mask(const std::string& symbol) override final {
            if (not program_.symbols_.contains(symbol))
                throw std::out_of_range(
                    std::format(
                        "get_mask: Symbol table does not contain symbol {}.",
                        symbol
                    )
                );

            size_t position = this->get_position(symbol);

            // Cannot generate a mask for a symbol that goes over the current word boundary
            if (program_.symbols_.at(symbol).size_ + position > 4)
                throw std::runtime_error("get_mask: Cannot generate a mask for a symbol crossing word boundary.");

            return cxxrtl::value<32>((0xFFFFFFFFu >> ((4 - program_.symbols_[symbol].size_) * 8)) << (position * 8));
        }

        virtual void assert_size_align(const std::string& symbol, uint32_t size, uint32_t alignment) override final {
            if (not program_.symbols_.contains(symbol))
                throw std::out_of_range(
                    std::format(
                        "assert_size_align: Symbol table does not contain symbol {}.",
                        symbol
                    )
                );

            const size_t symbol_size = get_size(symbol);
            if (symbol_size != size)
                throw std::runtime_error(
                    std::format(
                        "assert_size_align: Symbol {} is of size {}, not {}.",
                        symbol,
                        symbol_size,
                        size
                    )
                );

            const size_t position = get_position(symbol);
            if (position != alignment)
                throw std::runtime_error(
                    std::format(
                        "assert_size_align: Symbol {} is of alignment {}, not {}.",
                        symbol,
                        position,
                        alignment
                    )
                );
        }

        virtual ~ProgramInterface() = default;

    // Disable constructor to prevent UB
    protected:
        ProgramInterface() = default;
};

#endif // PROGRAM_SETUP_H
