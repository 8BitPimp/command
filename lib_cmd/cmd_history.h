#pragma once
#include "cmd.h"

struct cmd_history_t : public cmd_t {

    cmd_history_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("history", cli, parent, user)
    {
        desc_ = "show all previously executed commands";
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out, cmd_baton_t user) override
    {
        (void)user;
        auto indent = out.indent(2);
        size_t num = parser_.history_.size();
        num ? --num : 0;
        for (const auto& itt : parser_.history_) {
            // dont print last thing
            if (&itt != &parser_.history_.back()) {
                break;
            }
            out.println("(-%02d) %s", (uint32_t)num, itt.c_str());
            --num;
        }
        return true;
    }
};
