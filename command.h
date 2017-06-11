#pragma once
#include <cassert>
#include <cstdarg>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

typedef std::vector<std::unique_ptr<struct cmd_t>> cmd_list_t;

typedef void* cmd_baton_t;

struct cmd_output_t {
    FILE* fd_;

    cmd_output_t()
        : fd_(stdout)
    {
    }

    void indent(const uint32_t num)
    {
        for (uint32_t i = 0; i < num; ++i) {
            fputc(' ', fd_);
        }
    }

    void print(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(fd_, fmt, args);
        va_end(args);
    }

    void println(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(fd_, fmt, args);
        va_end(args);
        eol();
    }

    void eol()
    {
        fputc('\n', fd_);
    }
};

struct cmd_token_t {

    cmd_token_t()
    {
    }

    cmd_token_t(const std::string& str)
        : token_(str)
    {
    }

    const std::string& get() const
    {
        return token_;
    }

    bool get(int32_t& out) const
    {
        const int32_t base = (token_.find("0x") == 0) ? 16 : 10;
        out = (int32_t)strtol(token_.c_str(), nullptr, base);
        return errno != ERANGE;
    }

    bool get(int64_t& out) const
    {
        const int base = (token_.find("0x") == 0) ? 16 : 10;
        out = (int64_t)strtoll(token_.c_str(), nullptr, base);
        return errno != ERANGE;
    }

    bool operator==(const std::string& rhs) const
    {
        return token_ == rhs;
    }

    bool operator==(const char* rhs) const
    {
        return token_ == rhs;
    }

    operator std::string() const
    {
        return token_;
    }

    const char* c_str() const
    {
        return token_.c_str();
    }

protected:
    std::string token_;
};

struct cmd_tokens_t {

    cmd_tokens_t()
    {
    }

    size_t size() const
    {
        return tokens_.size();
    }

    bool flag(const std::string& name) const
    {
        return !(flags_.find(name) == flags_.end());
    }

    bool pair(const std::string& name, cmd_token_t & out) const
    {
        auto itt = pairs_.find(name);
        if (itt == pairs_.end()) {
            return false;
        } else {
            return (out = itt->second), true;
        }
    }

    void push(const std::string& input)
    {
        if (input.find("--") == 0) {
            stage_pair_.first.clear();
            flags_.insert(input);
            return;
        }
        if (input.find("-") == 0) {
            stage_pair_.first = input;
            return;
        }
        if (stage_pair_.first.empty()) {
            tokens_.push_back(input);
            return;
        } else {
            stage_pair_.second = input;
            pairs_.insert(stage_pair_);
            stage_pair_.first.clear();
            return;
        }
    }

    bool empty() const
    {
        return tokens_.empty();
    }

    const cmd_token_t& front() const
    {
        return tokens_.front();
    }

    void pop()
    {
        if (!tokens_.empty()) {
            tokens_.pop_front();
        }
    }

    bool has(uint32_t index) const
    {
        return tokens_.size() > index;
    }

protected:
    // staging area for pairs
    std::pair<std::string, cmd_token_t> stage_pair_;
    // basic token arguments
    std::deque<cmd_token_t> tokens_;
    // key value pair arguments
    std::map<std::string, cmd_token_t> pairs_;
    // switch flag
    std::set<std::string> flags_;
};

struct cmd_t {
    const char* const name_;
    struct cmd_parser_t& parser_;
    cmd_baton_t user_;
    cmd_t* parent_;
    cmd_list_t sub_;

    cmd_t(const char* name,
        cmd_parser_t& parser,
        cmd_baton_t user = nullptr)
        : name_(name)
        , parser_(parser)
        , user_(user)
        , parent_(nullptr)
    {
    }

    template <typename type_t>
    type_t* add_sub_command()
    {
        return add_sub_command<type_t>(user_);
    }

    template <typename type_t>
    type_t* add_sub_command(cmd_baton_t user)
    {
        auto temp = std::unique_ptr<type_t>(new type_t(parser_, user));
        temp->parent_ = this;
        sub_.push_back(std::move(temp));
        return (type_t*)sub_.rbegin()->get();
    }

    virtual bool on_execute(const cmd_tokens_t& tok, cmd_output_t& out)
    {
        int cmd_levenshtein(const char* s1, const char* s2);
        static const int FUZZYNESS = 3;
        const bool have_subcomands = !sub_.empty();
        if (!have_subcomands) {
            // an empty command is a bit weird
            return false;
        }
        const bool have_tokens = !tok.empty();
        if (have_tokens) {
            const char* tok_front = tok.front().c_str();
            std::vector<cmd_t*> list;
            for (const auto& i : sub_) {
                if (cmd_levenshtein(i->name_, tok.front().c_str()) < FUZZYNESS) {
                    list.push_back(i.get());
                }
            }
            out.println("  no subcommand '%s'", tok_front);
            if (!list.empty()) {
                out.println("  did you meen:");
                for (cmd_t* cmd : list) {
                    out.println("    %s", cmd->name_);
                }
            }
        } else {
            print_sub_commands(out);
        }
        return true;
    };

    void get_command_path(std::string& out) const
    {
        if (parent_) {
            parent_->get_command_path(out);
            out.append(" ");
        }
        out.append(name_);
    }

protected:
    void print_usage(cmd_output_t& out, const char* args)
    {
        std::string path;
        get_command_path(path);
        out.println("  %s [%s]", path.c_str(), args);
    }

    void print_cmd_list(const cmd_list_t& list, cmd_output_t& out) const
    {
        for (const auto& cmd : list) {
            out.println("  %s", cmd->name_);
        }
    }

    void print_sub_commands(cmd_output_t& out) const
    {
        print_cmd_list(sub_, out);
    }
};

struct cmd_parser_t {
    void* user_;
    cmd_list_t sub_;
    std::map<std::string, cmd_t*> alias_;
    std::vector<std::string> history_;

    cmd_parser_t(cmd_baton_t user = nullptr)
        : user_(user)
    {
        history_.push_back("help");
    }

    const std::string& last_cmd() const
    {
        return history_.back();
    }

    template <typename type_t>
    type_t* add_command()
    {
        return add_command<type_t>(user_);
    }

    template <typename type_t>
    type_t* add_command(void* user)
    {
        std::unique_ptr<type_t> temp(new type_t(*this, user));
        sub_.push_back(std::move(temp));
        return (type_t*)sub_.rbegin()->get();
    }

    /* execute expression */
    bool execute(const std::string& expr, cmd_output_t& output);

    /* find partial substring */
    bool find(const std::string& expr,
        std::vector<std::string>& out);

    /* alias control */
    bool alias_add(cmd_t* cmd, const std::string& alias);
    bool alias_remove(const std::string& alias);
    bool alias_remove(const cmd_t* cmd);

    cmd_t* alias_find(const std::string& alias) const
    {
        auto itt = alias_.find(alias);
        return itt == alias_.end() ? nullptr : itt->second;
    }
};

struct cmd_help_t : public cmd_t {

    struct cmd_help_tree_t : public cmd_t {
        cmd_help_tree_t(cmd_parser_t& cli, void* user)
            : cmd_t("tree", cli, user)
        {
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

        virtual bool on_execute(const cmd_tokens_t& tok, cmd_output_t& out)
        {
            walk(parser_.sub_, out, 2);
            return true;
        }
    };

    cmd_help_t(cmd_parser_t& cli, void* user)
        : cmd_t("help", cli, user)
    {
        add_sub_command<cmd_help_tree_t>();
    }

    virtual bool on_execute(const cmd_tokens_t& tok, cmd_output_t& out)
    {
        print_cmd_list(parser_.sub_, out);
        return true;
    }
};

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
