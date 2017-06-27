#pragma once
#include "cmd.h"

struct cmd_history_t : public cmd_t {

    cmd_history_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("history", cli, parent, user)
    {
        desc_ = "show all previously executed commands";
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        auto indent = out.indent(2);
        size_t num = parser_.history_.size();
        for (const auto& itt : parser_.history_) {
            out.println("(-%02d) %s", (uint32_t)num, itt.c_str());
            --num;
        }
        return true;
    }
};
