#include <string>
#include <vector>
#include <cstdint>

#include "../lib_cmd/cmd.h"

struct test_base_t {

    const char* const name;

    test_base_t(const char* const n)
        : name(n)
    {
    }

    virtual bool run() = 0;
};

struct test_store_t {

    static std::vector<test_base_t*> tests;

    static void add_test(test_base_t* test)
    {
        tests.push_back(test);
    }
};

#define CHECK(CND)        \
    {                     \
        if (!(CND))       \
            return false; \
    }
