#pragma once
#include "cmd.h"

struct cmd_help_t : public cmd_t {

    struct cmd_help_tree_t : public cmd_t {
        cmd_help_tree_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("tree", cli, parent, user)
        {
            desc_ = "list all commands and their sub commands";
            parser_.history_.push_back("help");
        }

        void walk(const cmd_list_t& list, cmd_output_t& out)
        {
            auto indent = out.indent(2);
            for (const auto& cmd : list) {
                out.println(cmd->name_);
                assert(cmd);
                if (!cmd->sub_.empty()) {
                    walk(cmd->sub_, out);
                }
            }
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
        {
            walk(parser_.sub_, out);
            return true;
        }
    };

    cmd_help_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("help", cli, parent, user)
    {
        desc_ = "list all root commands";
        add_sub_command<cmd_help_tree_t>();
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        print_cmd_list(parser_.sub_, out);
        return true;
    }
};
