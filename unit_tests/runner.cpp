#include "runner.h"
#include <cassert>
#include <cstdio>

std::vector<test_base_t*> test_store_t::tests;

int main(int argc, char** args)
{
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
