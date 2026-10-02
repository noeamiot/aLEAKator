#ifndef UTEST_H
#define UTEST_H

#include <iostream>

// Use traps that are never removed by NDEBUG instead of asserts
inline void trap_check(bool cond) {
    if (!cond) {
        std::cerr << "trap_check failed" << std::endl;
        __builtin_trap();
    }
}

#endif // UTEST_H
