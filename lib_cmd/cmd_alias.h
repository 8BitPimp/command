#pragma once
#include "cmd.h"

struct cmd_alias_t : public cmd_t {

    struct cmd_alias_add_t : public cmd_t {
        cmd_alias_add_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("add", cli, parent, user)
        {
            usage_ = "name cmd [cmd ...]";
            desc_ = "alias a command with a single name";
        }

        static cmd_t* cmd_find(const cmd_tokens_t& tok, const cmd_list_t* list)
        {
            cmd_t* cmd = nullptr;
            for (const cmd_token_t& token : tok.tokens.raw_) {
                cmd = nullptr;
                if (list == nullptr) {
                    return nullptr;
                }
                for (const auto& list_cmd : *list) {
                    if (token.get() == list_cmd->name_) {
                        cmd = list_cmd.get();
                        break;
                    }
                }
                if (cmd == nullptr) {
                    break;
                }
                if (!cmd->sub_.empty()) {
                    list = &(cmd->sub_);
                } else {
                    list = nullptr;
                }
            }
            return cmd;
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out, cmd_baton_t user) override
        {
            if (!tok.tokens.empty()) {
                // pop the name token
                cmd_token_t name = tok.tokens.front();
                tok.tokens.pop();
                // lookup a command for the remaining tokens
                cmd_t* cmd = cmd_find(tok, &(parser_.sub_));
                if (cmd == nullptr) {
                    auto ident = out.indent(2);
                    return cmd_locale_t::unable_to_find_cmd(out, name.c_str()), false;
                }
                parser_.alias_add(cmd, name);
                return true;
            }
            return on_usage(out, user), false;
        }
    };

    struct cmd_alias_remove_t : public cmd_t {
        cmd_alias_remove_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("remove", cli, parent, user)
        {
            usage_ = "name";
            desc_ = "remove a previously registered alias";
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out, cmd_baton_t user) override
        {
            (void)user, (void)out;
            for (const cmd_token_t& token : tok.tokens()) {
                parser_.alias_remove(token);
            }
            return true;
        }
    };

    struct cmd_alias_list_t : public cmd_t {
        cmd_alias_list_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("list", cli, parent, user)
        {
            desc_ = "list all registered aliases";
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out, cmd_baton_t user) override
        {
            auto indent = out.indent(2);
            cmd_locale_t::num_aliases(out, parser_.alias_.size());
            indent.add(2);
            std::string path;
            for (auto itt : parser_.alias_) {
                const cmd_t* cmd = itt.second;
                path.clear();
                cmd->get_command_path(path);
                out.println("%8s - %s", itt.first.c_str(), path.c_str());
            }
            return true;
        }
    };

    cmd_alias_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
        : cmd_t("alias", cli, parent, user)
    {
        add_sub_command<cmd_alias_add_t>(user);
        add_sub_command<cmd_alias_remove_t>(user);
        add_sub_command<cmd_alias_list_t>(user);
        desc_ = "manage command aliases";
    }
};
