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
std::bitset<64> stability(const cxxrtl::value<Bits>& val) {
    std::bitset<64> ret;
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
    cxxrtl::value<50> a{0x1u, 0x0u};
    cxxrtl::value<50> b{0x1u, 0x0u};
    cxxrtl::value<50> c;
    cxxrtl::value<1> sel{0x1u};

    std::cout << "Const initialized A, B: " << stability(a) << ", " << stability(b) << std::endl;
    std::cout << "Default initialized C: " << stability(c) << std::endl;

    // Additions 
    // Add and b fully stable
    c = a.add(b);
    assert(stability(c) == 0x0003FFFFFFFFFFFFull);
    std::cout << "Addition fully stable bit c: " << stability(c) << std::endl;

    a.stability[1] = 0x0002FFFFull;
    c = a.add(b);
    assert(stability(c) == 0x0000000000000000ull);
    std::cout << "Addition one unstable bit c: " << stability(c) << std::endl;

    // Restore full stability on a and remove one stab bit in b
    a.stability[1] = 0x0003FFFFull;
    b.stability[0] = 0xFFFEFFFFull;
    c = a.add(b);
    assert(stability(c) == 0x0000000000000000ull);
    std::cout << "Addition one unstable bit c: " << stability(c) << std::endl;

    // Substractions 
    // Add and b fully stable
    b.stability[0] = 0xFFFFFFFFull;
    c = a.sub(b);
    assert(stability(c) == 0x0003FFFFFFFFFFFFull);
    std::cout << "Substraction fully stable bit c: " << stability(c) << std::endl;

    a.stability[1] = 0x0002FFFFull;
    c = a.sub(b);
    assert(stability(c) == 0x0000000000000000ull);
    std::cout << "Substraction one unstable bit c: " << stability(c) << std::endl;

    // Restore full stability on a and remove one stab bit in b
    a.stability[1] = 0x0003FFFFull;
    b.stability[0] = 0xFFFEFFFFull;
    c = a.sub(b);
    assert(stability(c) == 0x0000000000000000ull);
    std::cout << "Substraction one unstable bit c: " << stability(c) << std::endl;

    // Negations 
    c = a.neg();
    assert(stability(c) == 0x0003FFFFFFFFFFFFull);
    std::cout << "Negation fully stable bit c: " << stability(c) << std::endl;

    a.stability[0] = 0xFFEFFFFFull;
    c = a.neg();
    assert(stability(c) == 0x0000000000000000ull);
    std::cout << "Negation partial unstable bit c: " << stability(c) << std::endl;

    // Multiplications
    // Restore full stability on a and b
    a.stability[0] = 0xFFFFFFFFull;
    b.stability[0] = 0xFFFFFFFFull;
    c = a.mul<50>(b);
    assert(stability(c) == 0x0003FFFFFFFFFFFFull);
    std::cout << "Multiplication fully stable bit c: " << stability(c) << std::endl;

    a.stability[1] = 0x0002FFFFull;
    c = a.mul<50>(b);
    assert(stability(c) == 0x0000000000000000ull);
    std::cout << "Multiplication one unstable bit c: " << stability(c) << std::endl;

    a.stability[1] = 0x0003FFFFull;
    b.stability[0] = 0xFFFEFFFFull;
    c = a.mul<50>(b);
    assert(stability(c) == 0x0000000000000000ull);
    std::cout << "Multiplication one unstable bit c: " << stability(c) << std::endl;

    // Multiplexor
    a.stability[1] = 0x0002AAAAull;
    a.stability[0] = 0xAAAAAAAAull;
    b.stability[1] = 0x00019999ull;
    b.stability[0] = 0x99999999ull;
    sel.stability[0] = 0x00000000ull;
    c = cxxrtl_yosys::symb_mux(sel, a, b);
    std::cout << stability(a) << std::endl;
    std::cout << stability(b) << std::endl;
    std::cout << stability(c) << std::endl;
    assert(stability(c) == 0x0000888888888888ull);
    std::cout << "Mux sel unstable bit c: " << stability(c) << std::endl;

    sel.stability[0] = 0x00000001ull;
    sel.set<bool, true>(0x0u);
    c = cxxrtl_yosys::symb_mux(sel, a, b);
    assert(stability(c) == 0x0001999999999999ull);
    std::cout << "Mux sel stable partially stable selected bit c: " << stability(c) << std::endl;

    sel.set<bool, true>(0x1u);
    c = cxxrtl_yosys::symb_mux(sel, a, b);
    assert(stability(c) == 0x0002AAAAAAAAAAAAull);
    std::cout << "Mux sel stable partially stable selected bit c: " << stability(c) << std::endl;

    verifMSICleanup();
}
