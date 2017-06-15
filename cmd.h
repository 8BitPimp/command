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

    void println(const char* fmt, va_list& args)
    {
        vfprintf(fd_, fmt, args);
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

    template <typename type_t>
    bool get(type_t& out) const
    {
        bool neg = false;
        uint64_t value = 0;
        if (!strtoll(token_.c_str(), value, neg)) {
            return false;
        }
        out = static_cast<type_t>(neg ? -value : value);
        return true;
    }

    bool operator==(const cmd_token_t& rhs) const
    {
        return token_ == rhs.get();
    }

    template <typename type_t>
    bool operator==(const type_t& rhs) const
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

    static bool strtoll(const char* in, uint64_t& out, bool& neg);

protected:
    std::string token_;
};

struct cmd_tokens_t {

    cmd_tokens_t()
    {
    }

    size_t token_size() const
    {
        return tokens_.size();
    }

    bool flag_get(const std::string& name) const
    {
        return !(flags_.find(name) == flags_.end());
    }

    bool pair_get(const std::string& name, cmd_token_t& out) const
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
        /* flush when input is empty */
        if (input.empty()) {
            if (!stage_pair_.first.empty()) {
                flags_.insert(stage_pair_.first);
                stage_pair_.first.clear();
            }
            return;
        }
        /* add to raw token set */
        raw_.push_back(input);
        /* if we have a flag or switch */
        if (input.find("-") == 0) {
            if (!stage_pair_.first.empty()) {
                flags_.insert(stage_pair_.first);
            }
            stage_pair_.first = input;
        } else {
            if (!stage_pair_.first.empty()) {
                pairs_[stage_pair_.first] = input;
                stage_pair_.first.clear();
            } else {
                tokens_.push_back(input);
            }
        }
    }

    bool token_empty() const
    {
        return tokens_.empty();
    }

    const cmd_token_t& token_front() const
    {
        return tokens_.front();
    }

    bool token_pop()
    {
        if (!tokens_.empty()) {
            assert(!raw_.empty());
            if (raw_.front() == tokens_.front()) {
                // pop prefixed tokens
                raw_.pop_front();
                tokens_.pop_front();
                return true;
            }
        }
        assert(!"FIXME: failed to pop");
        return false;
    }

    /* test if a token exists */
    bool token_exists(uint32_t index) const
    {
        return tokens_.size() > index;
    }

    const std::deque<cmd_token_t>& tokens() const
    {
        return tokens_;
    }

    const std::map<std::string, cmd_token_t>& pairs() const
    {
        return pairs_;
    }

    const std::set<std::string>& flags() const
    {
        return flags_;
    }

    const std::deque<cmd_token_t>& raw() const
    {
        return raw_;
    }

    const bool token_find(const std::string& in) const
    {
        for (const cmd_token_t& tok : tokens_) {
            if (tok == in) {
                return true;
            }
        }
        return false;
    }

protected:
    // raw tokens
    std::deque<cmd_token_t> raw_;
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
    // command name
    const char* const name_;
    // the owning command parser
    struct cmd_parser_t& parser_;
    // user data
    cmd_baton_t user_;
    // parent comand
    cmd_t* parent_;
    // sub command list
    cmd_list_t sub_;
    // command usage
    const char* usage_;

    /* mandatory constructor */
    cmd_t(const char* name,
        cmd_parser_t& parser,
        cmd_baton_t user = nullptr)
        : name_(name)
        , parser_(parser)
        , user_(user)
        , parent_(nullptr)
        , sub_()
        , usage_(nullptr)
    {
    }

    /* add a new subcommand */
    template <typename type_t>
    type_t* add_sub_command()
    {
        return add_sub_command<type_t>(user_);
    }

    /* add a new subcommand */
    template <typename type_t>
    type_t* add_sub_command(cmd_baton_t user)
    {
        auto temp = std::unique_ptr<type_t>(new type_t(parser_, user));
        temp->parent_ = this;
        sub_.push_back(std::move(temp));
        return (type_t*)sub_.rbegin()->get();
    }

    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out);

    /* return hierarchy of parent commands */
    void get_command_path(std::string& out) const
    {
        if (parent_) {
            parent_->get_command_path(out);
            out.append(" ");
        }
        out.append(name_);
    }

    /* print command usage */
    virtual bool on_usage(cmd_output_t& out) const
    {
        std::string path;
        get_command_path(path);
        out.print("  usage: %s %s", path.c_str(), usage_ ? usage_ : "");
        out.eol();
        return usage_ != nullptr;
    }

protected:
    bool alias_add(const std::string& name);

    bool error(cmd_output_t& out, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        out.println(fmt, args);
        va_end(args);
        return false;
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

    /* add new command */
    template <typename type_t>
    type_t* add_command()
    {
        return add_command<type_t>(user_);
    }

    /* add new command */
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

    static uint32_t levenshtein(const char* s1, const char* s2);
};
