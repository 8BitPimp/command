#include "lib_cmd/cmd_echo.h"
#include "lib_cmd/lib_cmd.h"
#include <array>

struct cmd_exit_t : public cmd_t {
    cmd_exit_t(cmd_parser_t& parser, cmd_t* parent, cmd_baton_t user)
        : cmd_t("exit", parser, parent, user)
    {
        desc_ = "exit the program";
    }

    bool on_execute(cmd_tokens_t& tok, cmd_output_t& out, cmd_baton_t user) override
    {
        (void)user;
        exit(0);
        return false;
    }
};

int main(const int argc, const char** args)
{
    // input stream buffer
    std::array<char, 1024> buffer;
    buffer.fill('\0');
    // create command parser and register command
    cmd_parser_t parser;
    parser.add_command<cmd_exit_t>();
    parser.add_command<cmd_help_t>();
    parser.add_command<cmd_alias_t>();
    parser.add_command<cmd_echo_t>();
    parser.add_command<cmd_expr_t>();
    parser.add_command<cmd_history_t>();
    // create output stream
    std::unique_ptr<cmd_output_t> out(cmd_output_t::create_output_stdio(stdout));
    // REPL (read-eval-print loop)
    out->print<false>("> ");
    while (fgets(buffer.data(), buffer.size(), stdin)) {
        const size_t size = strnlen(buffer.data(), buffer.size());
        buffer.data()[size ? size - 1 : 0] = '\0';
        std::string string(buffer.data(), buffer.size());
        if (string.empty()) {
            break;
        }
        if (!parser.execute(string, out.get(), nullptr)) {
        }
        out->print<false>("> ");
    }
    // exit
    return 0;
}
