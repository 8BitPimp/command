#include "runner.h"
#include <cassert>
#include <cstdio>

std::vector<test_base_t*> test_store_t::tests;

#define TEST(NAME) { \
        test_base_t* NAME(void); \
        test_store_t::add_test(NAME()); \
    }

void init() {
    TEST(init_test_parent);
    TEST(init_test_1);
    TEST(init_test_2);
    TEST(init_test_strtoll);
}

int main(int argc, char** args)
{
    init();

    int fails = 0;

    for (test_base_t* test : test_store_t::tests) {
        assert(test);
        printf("%16s - ", test->name);
        bool ret = test->run();
        printf("%s\n", ret ? "pass" : "fail");
        fails += ret ? 0 : 1;
    }

    if (fails) {
        fgetc(stdin);
    }
    return fails;
}
