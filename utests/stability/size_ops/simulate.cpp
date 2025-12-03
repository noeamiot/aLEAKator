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
    cxxrtl::value<1> a{0x1u};
    cxxrtl::value<1> b{0x1u};
    cxxrtl::value<33> c;
    cxxrtl::value<25> d;
    cxxrtl::value<64> e;
    cxxrtl::value<64> f;
    cxxrtl::value<64> g;

    std::cout << "Const initialized A, B: " << stability(a) << ", " << stability(b) << std::endl;
    std::cout << "Default initialized C,D,E: " << stability(c) << ", " << stability(d) << ", " << stability(e) << std::endl;

    // Stable zero extend (sign does not matter)
    c = a.zext<33>();
    assert(stability(c) == 0x00000001FFFFFFFFull);
    std::cout << "Zext stable a to 33 bits c: " << stability(c) << std::endl;

    // Unstable zero extend (sign does not matter)
    a.stability[0] = 0x0u;
    c = a.zext<33>();
    assert(stability(c) == 0x00000001FFFFFFFEull);
    std::cout << "Zext unstable a to 33 bits c: " << stability(c) << std::endl;

    // Partially stable zero extend
    d.stability[0] = 0x0FF00FE;
    c = d.zext<33>();
    assert(stability(c) == 0x00000001FEFF00FEull);
    std::cout << "Zext d to 33 bits c: " << stability(c) << std::endl;

    // Another partially stable zero extend with empty chunks
    d.stability[0] = 0x00F00F0;
    e = d.zext<64>();
    assert(stability(e) == 0xFFFFFFFFFE0F00F0ull);
    std::cout << "Zext d to 64 bits e: " << stability(e) << std::endl;

    // Stable right zero extend (sign does not matter)
    a.stability[0] = 0x1u;
    c = a.rzext<33>();
    assert(stability(c) == 0x00000001FFFFFFFFull);
    std::cout << "rZext stable a to 33 bits c: " << stability(c) << std::endl;

    // Unstable right zero extend (sign does not matter)
    a.stability[0] = 0x0u;
    c = a.rzext<33>();
    assert(stability(c) == 0x00000000FFFFFFFFull);
    std::cout << "rZext unstable a to 33 bits c: " << stability(c) << std::endl;

    // Partially stable right zero extend
    d.stability[0] = 0x1FF00FE;
    c = d.rzext<33>();
    assert(stability(c) == 0x00000001FF00FEFFull);
    std::cout << "rZext d to 33 bits c: " << stability(c) << std::endl;

    // Another partially stable right zero extend with empty chunks
    d.stability[0] = 0x10F00F0;
    e = d.rzext<64>();
    assert(stability(e) == 0x8780787FFFFFFFFFull);
    std::cout << "rZext d to 64 bits e: " << stability(e) << std::endl;

    // Stable sign extend (sign does not matter)
    d.stability[0] = 0x1FF00FF;
    c = d.sext<33>();
    assert(stability(c) == 0x00000001FFFF00FFull);
    std::cout << "Sext stable MSB d to 33 bits c: " << stability(c) << std::endl;

    // Unstable sign extend (sign does not matter)
    d.stability[0] = 0x0FF00FF;
    c = d.sext<33>();
    assert(stability(c) == 0x0000000000FF00FFull);
    std::cout << "Sext unstable a to 33 bits c: " << stability(c)  << std::endl;

    // Partially stable trunc
    c.stability[0] = 0xFE00FFFFu; // Only the lower FFFF will remain
    c.stability[1] = 0x1u; // This will be split
    d = c.trunc<25>();
    assert(stability(d) == 0x000000000000FFFFull);
    std::cout << "Trunc stable on last 16 bits c to 25 bits d: " << stability(d) << std::endl;

    // Partially stable rtrunc
    c.stability[0] = 0xFE00FFFFu; // The lower FF will be split
    c.stability[1] = 0x1u;
    d = c.rtrunc<25>();
    assert(stability(d) == 0x0000000001FE00FFull);
    std::cout << "rTrunc stable on last 16 bits c to 25 bits d: " << stability(d) << std::endl;

    // Blit
    c.stability[0] = 0xFF11AAFFu;
    c.stability[1] = 0x0u;
    e.stability[0] = 0xAA0FF001ull;
    e.stability[1] = 0x0000000Bull;
    f = e.blit<34, 2>(c);
    assert(stability(f) == 0x0000000BFC46ABFDull);
    std::cout << "Blit c: " << stability(c) << std::endl;
    std::cout << "Blit e: " << stability(e) << std::endl;
    std::cout << "Blit f: " << stability(f) << std::endl;

    // Repeat unstable
    a.stability[0] = 0x0u;
    g = a.repeat<64>();
    assert(stability(g) == 0x0000000000000000ull);
    std::cout << "Repeat unstable to 64 bits g: " << stability(g) << std::endl;

    // Repeat stable
    a.stability[0] = 0x1u;
    g = a.repeat<64>();
    assert(stability(g) == 0xFFFFFFFFFFFFFFFFull);
    std::cout << "Repeat stable to 64 bits g: " << stability(g) << std::endl;
    verifMSICleanup();
}
