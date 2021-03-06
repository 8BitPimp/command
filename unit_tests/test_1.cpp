#include "runner.h"

namespace {
struct cmd_test_t : public cmd_t {
    uint32_t* user_;

    cmd_test_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("test", cli, parent, user)
        , user_(static_cast<uint32_t*>(user))
    {
        assert(user);
        *user_ *= 7; // 7*2 = 14
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out, cmd_baton_t user) override
    {
        (void)user;
        *user_ *= 3; // 14*3 = 42
        return true;
    }
};

struct test_t : public test_base_t {

    test_t()
        : test_base_t(__FILE__)
    {
    }

    virtual bool run() override
    {
        uint32_t user_data = 2;
        cmd_parser_t parser;
        parser.add_command<cmd_test_t>(&user_data);
        cmd_output_t* output = cmd_output_t::create_output_dummy();
        bool ret = parser.execute("test", output, nullptr);
        CHECK(ret && (user_data == 42));
        return true;
    }
};
} // namespace {}

test_base_t* init_test_1()
{
    return new test_t();
}
