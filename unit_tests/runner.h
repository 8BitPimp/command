#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../lib_cmd/cmd.h"

struct test_base_t {

    const char* name;

    test_base_t(const char* n)
        : name(__ptr_or(strrchr(n, '\\'), n))
    {
    }

    virtual bool run() = 0;

protected:
    template <typename type_t>
    constexpr type_t* __ptr_or(type_t* a, type_t* b)
    {
        return a ? a : b;
    }
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
