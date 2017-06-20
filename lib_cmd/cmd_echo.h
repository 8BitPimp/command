#pragma once
#include "cmd.h"

struct cmd_echo_t : public cmd_t {

    cmd_echo_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("echo", cli, parent, user)
    {
        usage_ = "arg [arg] [...]";
        desc_ = "echo cmd_t args for debugging";
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        cmd_output_t::indent_t indent = out.indent_push(2);
        if (!tok.tokens().empty()) {
            std::string tokens;
            for (const cmd_token_t& token : tok.tokens()) {
                tokens.append(token);
                tokens.append(1, ' ');
            }
            out.println(true, "tokens: %s", tokens.c_str());
        }
        if (!tok.flags().empty()) {
            std::string flags;
            for (const cmd_token_t& flag : tok.flags()) {
                flags.append(flag);
                flags.append(1, ' ');
            }
            out.println(true, " flags: %s", flags.c_str());
        }
        if (!tok.pairs().empty()) {
            out.print(true, " pairs: ");
            std::string pair;
            for (const auto& pair : tok.pairs()) {
                out.print(true, "%s:%s ", pair.first.c_str(), pair.second.c_str());
            }
            out.eol();
        }
        if (!tok.raw().empty()) {
            out.print(true, "   raw: ");
            for (const cmd_token_t& token : tok.raw()) {
                out.print(true, "%s ", token.c_str());
            }
            out.eol();
        }
        return true;
    }
};
