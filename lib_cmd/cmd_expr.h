#pragma once
#include <cassert>
#include <cstdint>

#include <map>
#include <memory>
#include <vector>

#include "cmd.h"

struct cmd_expr_t : public cmd_t {

    struct cmd_expr_set_t : public cmd_t {

        cmd_expr_set_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("set", cli, parent, user)
        {
            usage_ = "[identifier] [value]";
            desc_ = "assign an identifier a value";
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
        {
            cmd_output_t::indent_t indent = out.indent(2);
            cmd_idents_t& idents = parser_.idents_;
            // parse identifier name
            std::string name;
            if (!tok.get(name)) {
                return out.println("identifier name required"), false;
            }
            assert(!name.empty());
            // parse identifier value
            uint64_t value;
            if (!tok.get(value)) {
                return out.println("value required"), false;
            }
            // set the identifier
            return (idents[name] = value), true;
        }
    };

    struct cmd_expr_remove_t : public cmd_t {

        cmd_expr_remove_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("remove", cli, parent, user)
        {
            usage_ = "[identifier]";
            desc_ = "erase an identifier";
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
        {
            cmd_output_t::indent_t indent = out.indent(2);
            cmd_idents_t& idents = parser_.idents_;
            // parse identifier name
            std::string name;
            if (!tok.get(name)) {
                return out.println("identifier name required"), false;
            }
            assert(!name.empty());
            // erase the identifier
            auto itt = idents.find(name);
            if (itt != idents.end()) {
                idents.erase(itt);
            } else {
                out.println("unable to find identifier '%s'", name.c_str());
            }
            return true;
        }
    };

    struct cmd_expr_eval_t : public cmd_t {

        cmd_expr_eval_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("eval", cli, parent, user)
        {
            usage_ = "[expression]";
            desc_ = "evaluate an algabreic expression";
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

        cmd_expr_list_t(cmd_parser_t& cli, cmd_t* parent, cmd_baton_t user)
            : cmd_t("list", cli, parent, user)
        {
            desc_ = "list all identifiers";
        }

        virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out) override
        {
            cmd_output_t::indent_t indent = out.indent(2);
            const cmd_idents_t& idents = parser_.idents_;
            out.println("%lld variables:", (uint64_t)idents.size());
            indent.add(2);
            for (const auto& itt : idents) {
                out.println("%8s 0x%llx", itt.first.c_str(), itt.second);
            }
            return true;
        }
    };

    cmd_expr_t(cmd_parser_t& cli, cmd_t* parent, void* user)
        : cmd_t("expr", cli, parent, user)
    {
        add_sub_command<cmd_expr_eval_t>();
        add_sub_command<cmd_expr_list_t>();
        add_sub_command<cmd_expr_set_t>();
        add_sub_command<cmd_expr_remove_t>();
        desc_ = "expression evaluation";
    }
};
