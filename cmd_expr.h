#pragma once
#include <cassert>
#include <cstdint>

#include <memory>
#include <map>
#include <vector>

#include "cmd.h"

struct cmd_expr_t : public cmd_t {

    cmd_expr_t(cmd_parser_t& cli, void* user)
        : cmd_t("expr", cli, user)
    {
    }

    bool join_expr(const cmd_tokens_t& tok, std::string& out)
    {
        out.clear();
        for (const cmd_token_t& token : tok.raw()) {
            out.append(token.get());
            out.append(1, ' ');
        }
        return true;
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override;
};
