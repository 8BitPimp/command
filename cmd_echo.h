#pragma once
#include "cmd.h"

struct cmd_echo_t : public cmd_t {

    cmd_echo_t(cmd_parser_t& cli, void* user)
        : cmd_t("echo", cli, user)
    {
        usage_ = R"([arg] [arg] ...
    Echo parsed command arguments for debugging purposes.
)";
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        if (!tok.tokens().empty()) {
            std::string tokens;
            for (const cmd_token_t& token : tok.tokens()) {
                tokens.append(token);
                tokens.append(1, ' ');
            }
            out.println("tokens: %s", tokens.c_str());
        }
        if (!tok.flags().empty()) {
            std::string flags;
            for (const cmd_token_t& flag : tok.flags()) {
                flags.append(flag);
                flags.append(1, ' ');
            }
            out.println(" flags: %s", flags.c_str());
        }
        if (!tok.pairs().empty()) {
            out.print(" pairs: ");
            std::string pair;
            for (const auto& pair : tok.pairs()) {
                out.print("%s:%s ", pair.first.c_str(), pair.second.c_str());
            }
            out.eol();
        }
        if (!tok.raw().empty()) {
            out.print("   raw: ");
            for (const cmd_token_t& token : tok.raw()) {
                out.print("%s ", token.c_str());
            }
            out.eol();
        }
        return true;
    }
};
