// test_util.h - tiny shared CHECK macro for the BULC test executables.
#pragma once
#include <iostream>

static int g_failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << ": " #cond "\n"; \
            g_failures++;                                                        \
        }                                                                        \
    } while (0)
#define TEST_MAIN_END()                                                          \
    do {                                                                         \
        if (g_failures) { std::cerr << g_failures << " check(s) failed\n"; return 1; } \
        std::cout << "all tests passed\n";                                       \
        return 0;                                                                \
    } while (0)
