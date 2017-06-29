#include "runner.h"

struct cmd_test_a_t : public cmd_t {

    static uint32_t exec;

    cmd_test_a_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("testa", cli, parent, user)
    {
        exec = 1;
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        exec <<= 1;
        return true;
    }
};

struct cmd_test_b_t : public cmd_t {

    static uint32_t exec;

    cmd_test_b_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("testb", cli, parent, user)
    {
        exec = 1;
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        exec <<= 1;
        return true;
    }
};

uint32_t cmd_test_a_t::exec;
uint32_t cmd_test_b_t::exec;

struct test_t : public test_base_t {

    test_t(const char* name)
        : test_base_t(name)
    {
        test_store_t::add_test(this);
    }

    virtual bool run() override
    {

        uint32_t& ea = cmd_test_a_t::exec;
        uint32_t& eb = cmd_test_b_t::exec;

        cmd_parser_t parser;
        CHECK(ea == 0 && eb == 0);

        parser.add_command<cmd_test_a_t>();
        CHECK(ea == 1 && eb == 0);

        parser.add_command<cmd_test_b_t>();
        CHECK(ea == 1 && eb == 1);

        cmd_output_t* stdio = cmd_output_t::create_output_stdio(stdout);
        parser.execute("test", stdio);
        CHECK(ea == 1 && eb == 1);

        parser.execute("testa", stdio);
        CHECK(ea == 2 && eb == 1);

        parser.execute("testb", stdio);
        CHECK(ea == 2 && eb == 2);

        return true;
    }
};

namespace {
test_t test{ "test2" };
} // namespace {}
