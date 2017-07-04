#include "runner.h"

namespace {

struct test_case_t {
    const char* in;
    uint64_t val;
    bool neg;
};

test_case_t eval[] = {
    { "0", 0, false },
    { "1", 1, false },
    { "1337", 1337, false },
    { "0xcafebeef", 0xcafebeef, false },
    { "0x3000ad", 0x3000ad, false },
    { "0x123456789", 0x123456789, false },
    { "0xabcdef", 0xabcdef, false },
    { "123456789", 123456789, false },
    { "00000", 0, false },
    { "0x0", 0, false },
    { "-0", 0, true },
    { "-1", 1, true },
    { "-1234", 1234, true },
    { "-0x1234", 0x1234, true }
};

template <typename type_t, size_t size>
size_t array_length(type_t (&a)[size])
{
    return size;
}

struct test_t : public test_base_t {

    test_t()
        : test_base_t(__FILE__)
    {
    }

    bool do_test(const char* in, uint64_t val, bool neg) const
    {
        uint64_t out_val;
        bool out_neg;
        bool ret = cmd_util_t::strtoll(in, out_val, out_neg);
        return (ret) && (val == out_val) && (out_neg == neg);
    }

    virtual bool run() override
    {
        for (size_t i = 0; i < array_length(eval); ++i) {
            auto& t = eval[i];
            if (!do_test(t.in, t.val, t.neg)) {
                return false;
            }
        }
        return true;
    }
};
} // namespace {}

test_base_t* init_test_strtoll()
{
    return new test_t();
}
