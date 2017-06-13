#pragma once
#include "cmd.h"

struct cmd_expr_t : public cmd_t {

    cmd_expr_t(cmd_parser_t& cli, void* user)
        : cmd_t("expr", cli, user)
    {
    }

    bool get_expr(const cmd_tokens_t& tok, std::string& out)
    {
        out.clear();
        for (const cmd_token_t& token : tok.raw()) {
            out.append(token.get());
            out.append(1, ' ');
        }
        return true;
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        std::string expr;
        if (!get_expr(tok, expr)) {
            return error(out, "  malformed expression");
        } else {
            out.println("  parsing: '%s'", expr.c_str());
        }

        out.println("  todo");
        return true;
    }
};
