#pragma once
#include <cassert>
#include <cstdint>

#include <map>
#include <memory>
#include <vector>

#include "cmd.h"

struct cmd_expr_t : public cmd_t {

    // persistent identifiers
    std::map<std::string, uint64_t> idents_;

    struct cmd_expr_eval_t : public cmd_t {
        cmd_expr_t& expr_;

        cmd_expr_eval_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("eval", cli, parent, user)
            , expr_(*static_cast<cmd_expr_t*>(parent))
        {
            alias_add("p");
        }

        bool join_expr(const cmd_tokens_t& tok, std::string& out) const
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

    struct cmd_expr_list_t : public cmd_t {
        cmd_expr_t& expr_;

        cmd_expr_list_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("list", cli, parent, user)
            , expr_(*static_cast<cmd_expr_t*>(parent))
        {
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
        {
            out.println("  %lld variables:", (uint64_t)expr_.idents_.size());
            for (const auto& itt : expr_.idents_) {
                out.println("    %8s 0x%llx", itt.first.c_str(), itt.second);
            }
            return true;
        }
    };

    cmd_expr_t(cmd_parser_t& cli, cmd_t* parent, void* user)
        : cmd_t("expr", cli, parent, user)
    {
        add_sub_command<cmd_expr_eval_t>();
        add_sub_command<cmd_expr_list_t>();
    }
};
