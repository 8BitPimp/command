#pragma once
#include <vector>
#include <memory>
#include <queue>
#include <string>
#include <map>
#include <set>
#include <cstdarg>

typedef std::vector<std::unique_ptr<struct command_t> > cmd_list_t;

struct cmd_output_t {
    FILE * fd_;

    cmd_output_t()
        : fd_(stdout)
    {
    }

    void print(const char * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fd_, fmt, args);
        va_end(args);
    }

    void println(const char * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fd_, fmt, args);
        va_end(args);
        eol();
    }

    void eol() {
        fputc('\n', fd_);
    }
};

template <typename type_t>
struct cmd_ret_t {

    cmd_ret_t() : valid_(false) {}
    cmd_ret_t(const type_t & value) : valid_(true), value_(value) {}

    const type_t & operator () () const {
        return value_;
    }

    type_t & operator () () {
        return value_;
    }

    operator bool () const {
        return valid_;
    }

protected:
    bool valid_;
    type_t value_;
};

struct cmd_token_t {

    cmd_token_t()
    {
    }

    cmd_token_t(const std::string & str)
        : token_(str)
    {
    }

    const std::string & get() const {
        return token_;
    }

    bool get(int32_t & out) const {
        const int32_t base = (token_.find("0x") == 0) ? 16 : 10;
        out = (int32_t) strtol(token_.c_str(), nullptr, base);
        return errno != ERANGE;
    }

    bool get(int64_t & out) const {
        const int base = (token_.find("0x") == 0) ? 16 : 10;
        out = (int64_t) strtoll(token_.c_str(), nullptr, base);
        return errno != ERANGE;
    }

    bool operator == (const std::string & rhs) const {
        return token_ == rhs;
    }

    bool operator == (const char * rhs) const {
        return token_ == rhs;
    }

    operator std::string () const {
        return token_;
    }

    const char * c_str() const {
        return token_.c_str();
    }

protected:
    std::string token_;
};

struct cmd_tokens_t {

    cmd_tokens_t()
    {}

    size_t size() const {
        return tokens_.size();
    }

    bool flag(const std::string & name) const {
        return !(flags_.find(name) == flags_.end());
    }

    cmd_ret_t<cmd_token_t> pair(const std::string & name) const {
        auto itt = pairs_.find(name);
        if (itt == pairs_.end()) {
            return cmd_ret_t<cmd_token_t>{};
        }
        else {
            return cmd_ret_t<cmd_token_t>{itt->second};
        }
    }

    void push(const std::string & input) {
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
        }
        else {
            stage_pair_.second = input;
            pairs_.insert(stage_pair_);
            stage_pair_.first.clear();
            return;
        }
    }

    bool empty() const {
        return tokens_.empty();
    }

    const cmd_token_t & front() const {
        return tokens_.front();
    }

    void pop() {
        if (!tokens_.empty()) {
            tokens_.pop_front();
        }
    }

    bool has(uint32_t index) const {
        return tokens_.size() > index;
    }

    cmd_ret_t<int32_t> find(const std::string & sub) const {
        for (size_t i=0; i<tokens_.size(); ++i) {
            if (tokens_[i] == sub) {
                return cmd_ret_t<int32_t>{int32_t(i)};
            }
        }
        return cmd_ret_t<int32_t>{-1};
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

struct cmd_value_t {
    // tagged type
};

struct cmd_arg_t {
    enum {
        e_flag,
        e_pair,
    } type_;
    bool optional_;
    const std::string value_, readme_;
};

struct command_t {
    const char* const name_;
    cmd_list_t sub_;
    struct command_parser_t& parser_;
    void* user_;
    std::vector<cmd_arg_t> args_;
    command_t* parent_;

    command_t(const char* name,
        command_parser_t& parser,
        void* user = nullptr)
        : name_(name)
        , parser_(parser)
        , user_(user)
        , parent_(nullptr)
    {
    }

    void add_argument(const cmd_arg_t & arg) {
        args_.push_back(arg);
    }

    template <typename type_t>
    type_t * add_sub_command(void* user = nullptr)
    {
        auto temp = std::unique_ptr<type_t>(new type_t(parser_, user));
        temp->parent_ = this;
        sub_.push_back(std::move(temp));
        return (type_t*) sub_.rbegin()->get();
    }

    virtual bool on_assign(const cmd_value_t&)
    {
        return false;
    };

    virtual bool on_execute(const cmd_tokens_t & tok, cmd_output_t & out)
    {
        int cmd_levenshtein(const char *s1, const char *s2);
        static const int FUZZYNESS = 3;
        const bool have_subcomands = !sub_.empty();
        if (!have_subcomands) {
            // an empty command is a bit weird
            return false;
        }
        const bool have_tokens = !tok.empty();
        if (have_tokens) {
            const char * tok_front = tok.front().c_str();
            std::vector<command_t*> list;
            for (const auto &i:sub_) {
                if (cmd_levenshtein(i->name_, tok.front().c_str()) < FUZZYNESS) {
                    list.push_back(i.get());
                }
            }
            out.println("  no subcommand '%s'", tok_front);
            if (!list.empty()) {
                out.println("  did you meen:");
                for (command_t * cmd:list) {
                    out.println("    %s", cmd->name_);
                }
            }
        }
        else {
            print_sub_commands(out);
        }
        return true;
    };

protected:
    void print_usage(cmd_output_t & out, const char * args) {
        std::string path;
        get_command_path(path);
        out.println("  %s [%s]", path.c_str(), args);
    }

    void get_command_path(std::string & out) {
        if (parent_) {
            parent_->get_command_path(out);
            out.append(" ");
        }
        out.append(name_);
    }

    void print_cmd_list(const cmd_list_t & list, cmd_output_t & out) const {
        for (const auto & cmd : list) {
            out.println("  %s", cmd->name_);
        }
    }

    void print_sub_commands(cmd_output_t & out) const {
        print_cmd_list(sub_, out);
    }
};

struct command_parser_t {
    cmd_list_t sub_;
    std::map<std::string, command_t*> alias_;
    void* user_;
    std::string last_cmd_;
    std::vector<std::string> history_;

    command_parser_t()
        : last_cmd_("help") {
    }

    template <typename type_t>
    type_t * add_command(void* user = nullptr)
    {
        std::unique_ptr<type_t> temp(new type_t(*this, user));
        sub_.push_back(std::move(temp));
        return (type_t*) sub_.rbegin()->get();
    }

    /* execute expression */
    bool execute(const std::string& expr, cmd_output_t & output);

    /* auto complete expression */
    bool auto_complete(std::string& in_out);

    /* find partial substring */
    bool find(const std::string& expr,
        std::vector<std::string>& out);

    /* find a command from an expression */
    command_t * find(const std::string & expr);

    /* alias control */
    bool alias_add(command_t * cmd, const std::string & alias);
    bool alias_remove(const std::string & alias);
    bool alias_remove(const command_t * cmd);
    command_t * alias_find(const std::string & alias) const {
        auto itt = alias_.find(alias);
        return itt==alias_.end() ? nullptr : itt->second;
    }
};

struct command_help_t : public command_t {

    command_help_t(command_parser_t & cli, void * user)
        : command_t("help", cli, user)
    {
    }

    virtual bool on_execute(const cmd_tokens_t & tok, cmd_output_t & out)
    {
        print_cmd_list(parser_.sub_, out);
        return true;
    }
};

struct command_alias_t : public command_t {

    struct command_alias_add_t: public command_t {
        command_alias_add_t(command_parser_t & cli, void * user)
            : command_t("add", cli, user)
        {
        }
        virtual bool on_execute(const cmd_tokens_t & tok, cmd_output_t & out) {
            return false;
        }
    };

    struct command_alias_remove_t: public command_t {
        command_alias_remove_t(command_parser_t & cli, void * user)
            : command_t("remove", cli, user)
        {
        }
        virtual bool on_execute(const cmd_tokens_t & tok, cmd_output_t & out) {
            return false;
        }
    };

    command_alias_t(command_parser_t & cli, void * user)
        : command_t("alias", cli, user)
    {
        add_sub_command<command_alias_add_t>(user);
        add_sub_command<command_alias_remove_t>(user);
    }

    virtual bool on_execute(const cmd_tokens_t & tok, cmd_output_t & out)
    {
        return true;
    }
};
