#include "runner.h"

namespace {
struct cmd_1_t : public cmd_t {
    uint32_t* user_;

    cmd_1_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("cmd_1", cli, parent, user)
        , user_(static_cast<uint32_t*>(user))
    {
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out, cmd_baton_t user) override
    {
        int* var = (int*)user;
        *var *= 2;
        return true;
    }
};

struct cmd_2_t : public cmd_t {
    uint32_t* user_;

    cmd_2_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("cmd_2", cli, parent, user)
        , user_(static_cast<uint32_t*>(user))
    {
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out, cmd_baton_t user) override
    {
        int* var = (int*)user;
        *var *= 2;
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
        cmd_parser_t parser_1;
        parser_1.add_command<cmd_1_t>(nullptr);

        cmd_parser_t parser_2;
        parser_1.add_command<cmd_2_t>(nullptr);
        parser_2.parent_ = &parser_1;

        cmd_output_t* output = cmd_output_t::create_output_dummy();
        CHECK(output);

        bool ret = true;
        uint32_t user = 1;

        ret = parser_1.execute("cmd_1", output, &user);
        CHECK(ret);

        // this cmd should not be found
        ret = parser_1.execute("cmd_2", output, &user);
        CHECK(ret == false);

        ret = parser_2.execute("cmd_1", output, &user);
        CHECK(ret);

        ret = parser_2.execute("cmd_2", output, &user);
        CHECK(ret);

        // this cmd should also not be found
        ret = parser_2.execute("cmd_unknown", output, &user);
        CHECK(ret);

        return true;
    }
};
} // namespace {}

test_base_t* init_test_parent()
{
    return new test_t();
}
