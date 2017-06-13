#include <array>

#include "cmd.h"
#include "cmd_alias.h"
#include "cmd_echo.h"
#include "cmd_expr.h"
#include "cmd_help.h"

struct cmd_exit_t : public cmd_t {
    cmd_exit_t(cmd_parser_t& parser, cmd_baton_t user)
        : cmd_t("exit", parser, user)
    {
        usage_ = "\n    Exit the program.";
    }

    bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        exit(0);
        return false;
    }
};

int main(const int argc, const char** args)
{
    std::array<char, 1024> buffer;
    buffer.fill('\0');

    cmd_parser_t parser;
    parser.add_command<cmd_exit_t>();
    parser.add_command<cmd_help_t>();
    parser.add_command<cmd_alias_t>();
    parser.add_command<cmd_echo_t>();
    parser.add_command<cmd_expr_t>();

    printf("> ");
    while (fgets(buffer.data(), buffer.size(), stdin)) {
        cmd_output_t out;
        const size_t size = strnlen(buffer.data(), buffer.size());
        buffer.data()[size ? size - 1 : 0] = '\0';
        std::string string(buffer.data(), buffer.size());
        if (string.empty()) {
            break;
        }
        if (!parser.execute(string, out)) {
        }
        printf("> ");
    }

    return 0;
}
