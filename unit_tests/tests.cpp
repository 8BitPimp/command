#include "../lib_cmd/cmd.h"
#include <vector>

typedef bool (*test_t)(void);

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

bool test_1()
{
    struct cmd_test_t : public cmd_t {
        uint32_t* user_;

        cmd_test_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("test", cli, parent, user)
            , user_(static_cast<uint32_t*>(user))
        {
            *user_ *= 7; // 7*2 = 14
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
        {
            *user_ *= 3; // 14*3 = 42
            return true;
        }
    };

    uint32_t user_data = 2;
    cmd_parser_t parser;
    parser.add_command<cmd_test_t>(&user_data);
    cmd_output_t* stdio = cmd_output_t::create_output_stdio(stdout);
    bool ret = parser.execute("test", stdio);
    return ret && (user_data == 42);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

int main(int argc, char** args)
{
    std::vector<test_t> tests;
    tests.push_back(test_1);

    bool pass = true;

    for (test_t test : tests) {
        pass &= test();
    }

    return pass ? 0 : 1;
}
