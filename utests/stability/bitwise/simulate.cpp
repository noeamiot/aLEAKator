#include <iostream>
#include <iomanip>
#include <bitset>
#include "cxxrtl/cxxrtl.h"
#include "verif_msi_pp.hpp"
#include "lss.h"

// Global to cxxrtl, must be defined but will produce no logs here
std::ofstream simulation_logger;

// Fancy way of saying 32 bits (only valid for non mul ops)
const int width_chunk = cxxrtl::chunk_traits<cxxrtl::chunk_t>::bits;

template <size_t Bits>
std::bitset<32> stability(const cxxrtl::value<Bits>& val) {
    std::bitset<32> ret;
    for (size_t i = 0; i < val.chunks; ++i) {
        for (size_t j = 0; j < width_chunk; ++j) {
            if (val.stability[i] & (1 << j)) {
                ret.set(i*(width_chunk)+j);
            }
        }
    }
    return ret;
}

int main(int argc, char *argv[]) {
    cxxrtl::value<1> a{0x1u};
    cxxrtl::value<1> b{0x1u};
    cxxrtl::value<1> c;

    std::cout << "Const initialized A, B: " << stability(a) << ", " << stability(b) << std::endl;
    std::cout << "Default initialized C: " << stability(c) << std::endl;

    // Both stable not considering absorbants
    b.stability[0] = 0x1u;
    a.stability[0] = 0x1u;
    c = cxxrtl_yosys::and_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") AND B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x000000000000001ull);
    c = cxxrtl_yosys::or_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") OR B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x000000000000001ull);
    c = cxxrtl_yosys::xor_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") XOR B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x000000000000001ull);

    // One non absorbant unstable
    b.stability[0] = 0x0u;
    a.stability[0] = 0x1u;
    a.set<bool, true>(0x1u);
    c = cxxrtl_yosys::and_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") AND B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000000ull);
    a.set<bool, true>(0x0u);
    c = cxxrtl_yosys::or_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") OR B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000000ull);
    c = cxxrtl_yosys::xor_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") XOR B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000000ull);

    // one absorbant unstable
    b.stability[0] = 0x0u;
    a.stability[0] = 0x1u;
    a.set<bool, true>(0x0u);
    c = cxxrtl_yosys::and_uu<1>(a, b);
    std::cout << "a (" << stability(a) << ") and b (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000001ull);
    a.set<bool, true>(0x1u);
    c = cxxrtl_yosys::or_uu<1>(a, b);
    std::cout << "a (" << stability(a) << ") or b (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000001ull);
    c = cxxrtl_yosys::xor_uu<1>(a, b);
    std::cout << "a (" << stability(a) << ") xor b (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000000ull);

    // Both non absorbant and unstable
    a.stability[0] = 0x0u;
    b.stability[0] = 0x0u;
    c = cxxrtl_yosys::and_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") AND B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000000ull);
    c = cxxrtl_yosys::or_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") OR B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000000ull);
    c = cxxrtl_yosys::xor_uu<1>(a, b);
    std::cout << "A (" << stability(a) << ") XOR B (" << stability(b) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x0000000000000000ull);


    // Not gate unstable
    a.stability[0] = 0x0u;
    c = cxxrtl_yosys::not_u<1>(a);
    std::cout << "NOT A (" << stability(a) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x00000000000000ull);

    // Not gate stable
    a.stability[0] = 0x1u;
    c = cxxrtl_yosys::not_u<1>(a);
    std::cout << "NOT A (" << stability(a) << ") : " << stability(c) << std::endl;
    assert(stability(c) == 0x00000000000001ull);
    verifMSICleanup();
}
