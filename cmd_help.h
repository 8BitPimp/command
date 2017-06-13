#pragma once
#include "cmd.h"

struct cmd_help_t : public cmd_t {

    struct cmd_help_tree_t : public cmd_t {
        cmd_help_tree_t(cmd_parser_t& cli, void* user)
            : cmd_t("tree", cli, user)
        {
            usage_ = R"(
  list all commands and their sub commands
)";
        }

        void walk(const cmd_list_t& list, cmd_output_t& out, uint32_t indent)
        {
            for (const auto& cmd : list) {
                out.indent(indent);
                out.println(cmd->name_);
                assert(cmd);
                if (!cmd->sub_.empty()) {
                    walk(cmd->sub_, out, indent + 2);
                }
            }
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
        {
            walk(parser_.sub_, out, 2);
            return true;
        }
    };

    cmd_help_t(cmd_parser_t& cli, void* user)
        : cmd_t("help", cli, user)
    {
        usage_ = R"(
  list all commands
)";
        add_sub_command<cmd_help_tree_t>();
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
    {
        print_cmd_list(parser_.sub_, out);
        return true;
    }
};
