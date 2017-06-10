#include <array>

#include "command.h"

struct cmd_exit_t: public cmd_t {
    cmd_exit_t(cmd_parser_t & parser, cmd_baton_t user)
        : cmd_t("exit", parser, user)
    {}

    bool on_execute(const cmd_tokens_t& tok, cmd_output_t& out) {
        return false;
    }
};

int main(const int argc, const char** args)
{
    std::array<char, 1024> buffer;
    buffer.fill('\0');

    cmd_parser_t parser;

    while (gets_s(buffer.data(), buffer.size())) {

        cmd_output_t out;

        std::string string(buffer.data(), buffer.size());
        if (string.empty()) {
            break;
        }

        if (!parser.execute(string, out)) {
        }
    }

    return 0;
}
