#pragma once
#include "cmd.h"

struct cmd_expr_t : public cmd_t {

    cmd_expr_t(cmd_parser_t& cli, void* user)
        : cmd_t("expr", cli, user)
    {
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        out.println("  todo");
        return true;
    }
};
