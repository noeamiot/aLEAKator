#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <filesystem>
namespace fs = std::filesystem;

#include "lss.h"

class Configuration {
    public:
        // Properties such as SNI are only supported on gadgets
        enum CircuitType { CPU, ACCELERATOR, GADGET };
        enum HigherOrderType { SPATIAL, TEMPORAL };

        std::filesystem::path working_path_;
        const CircuitType circuit_type_;
        const std::string program_;
        std::string subprogram_{}; // Set for CPUs, empty otherwise

        bool BIT_VERIF_ = true;
        bool USE_STABILITY_ = true;
        bool REMOVE_FALSE_NEGATIVE_ = false;
        bool EXIT_AT_FIRST_LEAKING_CYCLE_ = false;
        bool EXIT_AT_FIRST_LEAK_ = false;

        bool VERIF_VALUE_WO_GLITCHES_ = false;
        bool VERIF_VALUE_W_GLITCHES_ = false;
        bool VERIF_TRANSITION_WO_GLITCHES_ = false;
        bool VERIF_TRANSITION_W_GLITCHES_ = false;

        bool TRANSITION_W_GLITCHES_OVER_APPROX_ = true;
        bool FORCE_VERIFY_ALL_ = false;

        bool DETAIL_LEAKS_INFORMATION_ = false;
        bool DETAIL_SHOW_EXPRESSION_ = false;
        bool TRACK_LEAKS_ = false;

        std::map<std::string, int> EXCEPTIONS_WORD_VERIF_;

        leaks::Properties SECURITY_PROPERTY_ = leaks::Properties::TPS;
        size_t ORDER_VERIF_ = 1;
        HigherOrderType HIGHER_ORDER_TYPE_ = HigherOrderType::SPATIAL;
        size_t SKIP_VERIF_CYCLES_ = 0;
        // For now this does include the reset cycles
        int64_t CYCLES_TO_VERIFY_ = std::numeric_limits<int64_t>::max();


    public:
        Configuration(int argc, char *argv[], CircuitType circuit_type, const std::string& program);
        void dump() const;
        friend std::ostream& operator<<(std::ostream& os, Configuration const& m);
    private:
        void init_working_path(const std::string& arg0);
};

#endif // CONFIGURATION_H
