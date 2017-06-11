#pragma once
#include "cmd.h"

struct cmd_alias_t : public cmd_t {

    struct cmd_alias_add_t : public cmd_t {
        cmd_alias_add_t(cmd_parser_t& cli, void* user)
            : cmd_t("add", cli, user)
        {
        }

        virtual bool on_execute(const cmd_tokens_t& tok, cmd_output_t& out)
        {
#if 0
            if (tok.size() < 2) {
                print_usage(out, "name command [...]");
                return false;
            }
            std::string cmd;
            if (!tok.concat(1, cmd)) {
                return false;
            }
#endif
            return false;
        }
    };

    struct cmd_alias_remove_t : public cmd_t {
        cmd_alias_remove_t(cmd_parser_t& cli, void* user)
            : cmd_t("remove", cli, user)
        {
        }

        virtual bool on_execute(const cmd_tokens_t& tok, cmd_output_t& out)
        {
            return false;
        }
    };

    struct cmd_alias_list_t : public cmd_t {
        cmd_alias_list_t(cmd_parser_t& cli, void* user)
            : cmd_t("list", cli, user)
        {
        }

        virtual bool on_execute(const cmd_tokens_t& tok, cmd_output_t& out)
        {
            out.println("  %d aliases", parser_.alias_.size());
            std::string path;
            for (auto itt : parser_.alias_) {
                const cmd_t* cmd = itt.second;
                path.clear();
                cmd->get_command_path(path);
                out.println("    %s - %s", itt.first.c_str(), path.c_str());
            }
            return true;
        }
    };

    cmd_alias_t(cmd_parser_t& cli, void* user)
        : cmd_t("alias", cli, user)
    {
        add_sub_command<cmd_alias_add_t>(user);
        add_sub_command<cmd_alias_remove_t>(user);
        add_sub_command<cmd_alias_list_t>(user);
    }
};
