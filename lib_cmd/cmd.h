#pragma once
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

typedef std::vector<std::unique_ptr<struct cmd_t>> cmd_list_t;
typedef std::map<std::string, uint64_t> cmd_idents_t;
typedef void* cmd_baton_t;

/// @brief cmd_util_t, utility functions for the command parser
///
struct cmd_util_t {

    /// @brief convert string to 64 bit integer
    ///
    /// @param in input string to parse as integer
    /// @param out output integer to receive contersion
    /// @param neg output boolean to be set if integer should be negated
    ///
    /// @return true if the conversion was successfull
    static bool strtoll(const char* in, uint64_t& out, bool& neg);

    /// @brief levenshtein string distance function
    ///
    /// @param s1 input string a
    /// @param s2 input string b
    ///
    /// @return edit distance between strings s1 and s2
    static uint32_t levenshtein(const char* a, const char* b);

    /// @brief partial substring match
    ///
    /// @return number of characters between str and sub that match or -1 if different
    static int32_t str_match(const char* str, const char* sub);
};

/// @brief cmd_output_t, command output interface base class
///
/// this class brokers all output text writing from cmd_t classes during command execution.
/// using a specific class we can ensure consisten output and eliminate output race conditions.
/// a versatile iterface also makes it easy to output text in a range of formats.
/// cmd_output_t is also a factory for derived classes with specialisations.
///
struct cmd_output_t {

    /// Create a cmd_output_t instance that will write directly to a file descriptor.
    ///
    /// @param fd, the file descriptior that all output will be written to.
    /// @return cmd_output_t instance.
    static cmd_output_t* create_output_stdio(FILE* fd);

    /// @brief indent_t, indent control helper class
    ///
    struct indent_t {

        indent_t(uint32_t* ptr, uint32_t indent)
            : ptr_(ptr)
            , restore_(*ptr)
        {
            assert(ptr);
            *ptr += indent;
        }

        ~indent_t()
        {
            assert(ptr_);
            *ptr_ = restore_;
        }

        void add(uint32_t num)
        {
            assert(ptr_);
            *ptr_ += num;
        }

    protected:
        uint32_t* ptr_;
        uint32_t restore_;
    };

    /// @brief guard_t, output mutex helper class
    ///
    struct guard_t {
        cmd_output_t& out_;

        guard_t() = delete;

        guard_t(cmd_output_t& out)
            : out_(out)
        {
            out.lock();
        }

        ~guard_t()
        {
            out_.unlock();
        }
    };

    /// @brief obtain scope guard for the output mutex
    ///
    /// @return scope guard object for the output mutex
    guard_t guard()
    {
        return guard_t(*this);
    }

    /// @brief constructor
    ///
    cmd_output_t()
        : indent_(2)
    {
    }

    /// @brief virtual destructor
    ///
    virtual ~cmd_output_t() {}

    /// @brief aquire the output mutex
    ///
    virtual void lock() = 0;

    /// @brief release the output mutex
    ///
    virtual void unlock() = 0;

    /// @brief push a new indentation level
    ///
    /// @return indent helper class
    indent_t indent(uint32_t next = 2)
    {
        return indent_t(&indent_, next);
    }

    template <bool INDENT = true>
    void print(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        print(INDENT, fmt, args);
        va_end(args);
    }

    template <bool INDENT = true>
    void println(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        println(INDENT, fmt, args);
        va_end(args);
    }

    virtual void print(bool indent, const char* fmt, va_list& args) = 0;
    virtual void println(bool indent, const char* fmt, va_list& args) = 0;

    virtual void eol() = 0;

protected:
    /// @brief current indentation level
    uint32_t indent_;
};

/// @brief cmd_locale_t, command locale text definitions
///
struct cmd_locale_t {

    static void possible_completions(cmd_output_t& out)
    {
        out.println("possible completions:");
    }

    static void invalid_command(cmd_output_t& out)
    {
        out.println("invalid command");
    }

    static void no_subcommand(cmd_output_t& out, const char* cmd)
    {
        out.println("no subcommand '%s'", cmd);
    }

    static void did_you_meen(cmd_output_t& out)
    {
        out.println("did you meen:");
    }

    static void not_val_or_ident(cmd_output_t& out)
    {
        out.println("return type not value or identifier");
    }

    static void unknown_ident(cmd_output_t& out, const char* ident)
    {
        out.println("unknown identifier '%s'", ident);
    }

    static void malformed_exp(cmd_output_t& out)
    {
        out.println("malformed expression");
    }

    static void error(cmd_output_t& out, const char* err)
    {
        out.println("error: %s", err);
    }

    static void usage(cmd_output_t& out, const char* path, const char* args, const char* desc)
    {
        out.println("usage: %s %s", path, args ? args : "");
        if (desc) {
            out.println("desc:  %s", desc);
        }
    }

    static void subcommands(cmd_output_t& out)
    {
        out.println("subcomands:");
    }

    static void unable_to_find_cmd(cmd_output_t& out, const char* cmd)
    {
        out.println("unable to find command '%s'", cmd);
    }

    static void num_aliases(cmd_output_t& out, uint64_t num)
    {
        num ? out.println("%d aliases:", (uint32_t)num) : out.println("no alises");
    }

    static void command_failed(cmd_output_t& out, const char* cmd)
    {
        out.println("  command failed: '%s'", cmd);
    }
};

/// @brief cmd_token_t, command arguement token
///
struct cmd_token_t {

    /// @brief constructor
    cmd_token_t() = default;

    /// @brief cmd_token_t consructor
    ///
    /// @param string token
    cmd_token_t(const std::string& string)
        : token_(string)
    {
    }

    /// @brief return token as a string
    ///
    /// @return underlying token string
    const std::string& get() const
    {
        return token_;
    }

    /// @brief get token as an integer
    ///
    /// @oaram output integer to store the conversion
    /// @return true if the token represents an integer
    template <typename type_t>
    bool get(type_t& out) const
    {
        bool neg = false;
        uint64_t value = 0;
        if (!cmd_util_t::strtoll(token_.c_str(), value, neg)) {
            return false;
        }
        out = static_cast<type_t>(neg ? 0 - value : value);
        return true;
    }

    /// @brief test for token equality
    ///
    /// @param rhs token to check equality with
    /// @return true if tokens are equal
    bool operator==(const cmd_token_t& rhs) const
    {
        return token_ == rhs.get();
    }

    /// @brief test for token equality
    ///
    /// @oaram rhs token to check equality with
    /// @return true if token is equal to rhs
    template <typename type_t>
    bool operator==(const type_t& rhs) const
    {
        return token_ == rhs;
    }

    /// @brief std::string cast operator
    ///
    /// @return token as string
    operator std::string() const
    {
        return token_;
    }

    /// @brief c string cast operator
    ///
    /// @return c string representataion of token
    const char* c_str() const
    {
        return token_.c_str();
    }

protected:
    std::string token_;
};

/// @breif cmd_tokens_t, command arguments token list
///
struct cmd_tokens_t {

#if 0
    //XXX: todo
    struct {

    } flags;

    struct {

    } pairs;

    struct {

    } tokens;
#endif

    /// @brief list of identifiers that can be substituted for tokens
    cmd_idents_t* idents_;

    /// @brief constructor
    ///
    /// @oaram idents list of identifiers to substitute tokens with
    cmd_tokens_t(cmd_idents_t* idents)
        : idents_(idents)
    {
    }

    /// @brief return number of tokens in the token list
    ///
    /// @return number of tokens in the token list
    size_t token_size() const
    {
        return tokens_.size();
    }

    /// @brief get the front most token from the token list
    ///
    /// @return true if the front most token could be retrived
    bool get(std::string& out)
    {
        if (tokens_.empty()) {
            return false;
        }
        out = tokens_.front().get();
        tokens_.pop_front();
        return true;
    }

    /// @brief get the front most token from the token list as an integer
    ///
    /// @param output to receive front most token
    /// @return true if the front most token could be retrived as an integer
    bool get(uint64_t& output)
    {
        if (tokens_.empty()) {
            return false;
        }
        if (!tokens_.front().get(output)) {
            return false;
        }
        tokens_.pop_front();
        return true;
    }

    /// @brief get the front most token from the token list
    ///
    /// @param output to receive front most token
    /// @return true if the front most token could be retrived
    bool get(cmd_token_t& out)
    {
        if (tokens_.empty()) {
            return false;
        }
        out = tokens_.front();
        tokens_.pop_front();
        return true;
    }

    /// @brief check if a flag was passed to the token list
    ///
    /// @return true if 'name' flag was passed as an argument
    bool flag_get(const std::string& name) const
    {
        return !(flags_.find(name) == flags_.end());
    }

    /// @brief retreive the argument to a passed token pair
    ///
    /// @return true if the pair was in the token list
    bool pair_get(const std::string& name, cmd_token_t& out) const
    {
        auto itt = pairs_.find(name);
        if (itt == pairs_.end()) {
            return false;
        } else {
            return (out = itt->second), true;
        }
    }

    /// @brief push a new token into this token list
    ///
    /// @param string token to push onto list
    void push(std::string input);

    /// @brief check if the token list is empty
    ///
    /// @return true if the token list is empty
    bool token_empty() const
    {
        return tokens_.empty();
    }

    /// @brief return a reference to the first token in the queue
    ///
    /// @return reference to first token
    const cmd_token_t& token_front() const
    {
        return tokens_.front();
    }

    /// @brief return a reference to the last token in the queue
    ///
    /// @return reference to last token
    const cmd_token_t& token_back() const
    {
        return tokens_.back();
    }

    /// @brief pop front most argument from token list
    ///
    /// @return true if front argument was popped
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

    /// @brief return queue of tokens
    ///
    /// @return queue of tokens
    const std::deque<cmd_token_t>& tokens() const
    {
        return tokens_;
    }

    /// @brief return map of parameter pairs
    ///
    /// @return map of parameter pairs
    const std::map<std::string, cmd_token_t>& pairs() const
    {
        return pairs_;
    }

    /// @brief Return a set containing all user supplied flags
    const std::set<std::string>& flags() const
    {
        return flags_;
    }

    /// @brief Return a list of raw tokens pushed to the token list
    ///
    /// @return queue of raw tokens pushed to this token list
    const std::deque<cmd_token_t>& raw() const
    {
        return raw_;
    }

    /// @brief Find a matching token in the token list
    ///
    /// @param in input string to try and locate in the token list
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
    /// @brief raw tokens
    std::deque<cmd_token_t> raw_;

    /// @brief staging area for pairs
    std::pair<std::string, cmd_token_t> stage_pair_;

    /// @brief basic token arguments
    std::deque<cmd_token_t> tokens_;

    /// @brief key value pair arguments
    std::map<std::string, cmd_token_t> pairs_;

    /// @brief switch flag
    std::set<std::string> flags_;
};

/// @brief cmd_t, the command base class.
///
/// this is the base command class that should be extended to handle custom commands.
/// a cmd_t can be a child of another command or inserted into a cmd_parser_t as a root command.
/// two key methods can be overriden, on_execute and on_usage which the cmd_parser_t will invoke based on user input.
///
struct cmd_t {

    /// @brief command name
    const char* const name_;

    /// @brief owning command parser
    struct cmd_parser_t& parser_;

    /// @brief opaque user data for this command
    cmd_baton_t user_;

    /// @brief the parent if this is a child command
    cmd_t* parent_;

    /// @brief list of child commands
    cmd_list_t sub_;

    /// @brief command argument string
    const char* usage_;

    /// @brief command description string
    const char* desc_;

    /// @brief cmd_t constructor.
    ///
    /// @param const char* name, the name of this command
    /// @param cmd_t* parent, the parent cmd_t instance
    /// @param cmd_baton_t user, the opaque user data passed to this cmd_t instance
    cmd_t(const char* name,
        cmd_parser_t& parser,
        cmd_t* parent,
        cmd_baton_t user = nullptr)
        : name_(name)
        , parser_(parser)
        , user_(user)
        , parent_(parent)
        , sub_()
        , usage_(nullptr)
        , desc_(nullptr)
    {
    }

    /// @brief Add child command to this command.
    ///
    /// instanciate and attach a new child command to this parent command
    /// supplying this commands user_ data during its construction.
    ///
    /// @ return the new cmd_t instance
    template <typename type_t>
    type_t* add_sub_command()
    {
        return add_sub_command<type_t>(user_);
    }

    /// @brief Add child command to this command.
    ///
    /// instanciate and attach a new child command to this parent command.
    ///
    /// @param user opaque user data to pass to the new child cmd_t.
    /// @return the new cmd_t instance.
    template <typename type_t>
    type_t* add_sub_command(cmd_baton_t user)
    {
        auto temp = std::unique_ptr<type_t>(new type_t(parser_, this, user));
        sub_.push_back(std::move(temp));
        return (type_t*)sub_.rbegin()->get();
    }

    /// @brief Command execution handler.
    ///
    /// virtual function that will be called when the user specifies is full path or an alias to this command.
    /// if this command is ternimal then any following input tokens will be passed to this handler.
    /// the caller provided output stream is also passed to the called command execution handler.
    ///
    /// @param tok token list of arguments supplied by the user.
    /// @param out text output stream for writing results to.
    /// @return true if the command executed successfully.
    virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out);

    /// Return string with hierarchy of parent commands.
    ///
    /// @param out the string to store output hierarchy.
    void get_command_path(std::string& out) const
    {
        if (parent_) {
            parent_->get_command_path(out);
            out.append(" ");
        }
        out.append(name_);
    }

    /// @brief Print command usage information to output stream.
    ///
    /// @param out output stream to print to
    /// @return true if usage was written successfully
    virtual bool on_usage(cmd_output_t& out) const
    {
        cmd_output_t::indent_t indent = out.indent(2);
        std::string path;
        get_command_path(path);
        cmd_locale_t::usage(out, path.c_str(), usage_, desc_);
        if (!sub_.empty()) {
            cmd_locale_t::subcommands(out);
            print_sub_commands(out);
        }
        return true;
    }

protected:
    /// @brief Add an alias for this command.
    ///
    /// bind a cmt_t instance to a single string token known as an alias.
    /// during command interpretation if the first token matches any alises then the associated cmd_t instance will be executed.
    ///
    /// @param name the string name of the alias to add.
    /// @return true if the alias was associated successfully.
    bool alias_add(const std::string& name);

    /// @brief Report an error condition to the output stream.
    ///
    /// @param out output stream.
    /// @param fmt input format string.
    /// @param ... variable args.
    /// @return false so it can be propagated via return statements easily.
    bool error(cmd_output_t& out, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        out.println(fmt, args);
        va_end(args);
        return false;
    }

    /// @brief Print a list of command names to an output stream.
    ///
    /// @param list input command list.
    /// @param out output stream to write command names to.
    void print_cmd_list(const cmd_list_t& list, cmd_output_t& out) const
    {
        cmd_output_t::indent_t indent = out.indent(2);
        for (const auto& cmd : list) {
            out.println("%s", cmd->name_);
        }
    }

    /// @brief Print a list of child commands to the output stream.
    ///
    /// @param out output stream to write command names to.
    void print_sub_commands(cmd_output_t& out) const
    {
        print_cmd_list(sub_, out);
    }
};

/// @brief cmd_parser_t, the command parser.
///
/// this type is the main workhorse of the command library.  it forms the root of the command hieararchy
/// it stores some state that can be accessed via all commands (alias_, idents_)
/// cmd_parser_t is responsible for parsing and dispatching user input to the appropriate command
///
struct cmd_parser_t {

    /// global user data passed to new subcommands unless overridden
    void* user_;

    /// root subcommand list
    cmd_list_t sub_;

    /// user input history
    std::vector<std::string> history_;

    /// map of alias names to command instances
    std::map<std::string, cmd_t*> alias_;

    /// expression identifier list
    cmd_idents_t idents_;

    /// @brief cmd_parser_t constructor.
    ///
    /// @param user opaque user data pointer passed from parent to child.
    /// @return user a global custom data pointer to be passed to any sub commands.
    cmd_parser_t(cmd_baton_t user = nullptr)
        : user_(user)
    {
    }

    /// @brief Get a string with the last user input to be executed.
    ///
    /// @return reference to the last
    const std::string& last_cmd() const
    {
        return history_.back();
    }

    /// @brief Add a new root command to the command parser.
    ///
    /// add a new root command to the command interpreter.
    /// the new subcommand instance will be passed the global cmd_parser_t user_ data
    ///
    /// @return instance of the newly created command.
    template <typename type_t>
    type_t* add_command()
    {
        return add_command<type_t>(user_);
    }

    /// @brief Add a new root command to the command parser.
    ///
    /// @param user the user data type to pass to the sub command.
    /// @return instance of the newly created command.
    template <typename type_t>
    type_t* add_command(cmd_baton_t user)
    {
        cmd_t* parent = nullptr;
        std::unique_ptr<type_t> temp(new type_t(*this, parent, user));
        sub_.push_back(std::move(temp));
        return (type_t*)sub_.rbegin()->get();
    }

    /// @brief Add a new root command to the command parser.
    ///
    /// Note: the command parameter will be moved so that ownership is
    /// transfered to the command parser.  command should not be accessed
    /// after calling this function and instead, the return instance should
    /// be used in its place.
    ///
    /// @param command a command instance to add to the parser
    /// @return the new command instance after moving the parameter
    cmd_t* add_command(cmd_t*& command)
    {
        std::unique_ptr<cmd_t> temp(std::move(command));
        sub_.push_back(std::move(temp));
        command = nullptr;
        return sub_.rbegin()->get();
    }

    /// @brief Execute expressions, calling the relevant cmd_t instances with arguments.
    ///
    /// @param a list of ';' delimited expression strings to execute.
    /// @param output output stream that can be written to during execution.
    /// @return true if the command executed successfully.
    bool execute(const std::string& expr, cmd_output_t* output);

    /// @brief Add a new parser alias for a cmd_t instance.
    ///
    /// @param cmd command instance for which to make an alias.
    /// @param alias name for the alias.
    /// @return true if that alias was added.
    bool alias_add(cmd_t* cmd, const std::string& alias);

    /// @brief Remove a previously registered command alias byt name.
    ///
    /// @param alias the string command alias to remove.
    /// @return true if the alias was removed.
    bool alias_remove(const std::string& alias);

    /// @brief Remove any previously registered command alises by target cmd_t.
    ///
    /// @param cmd the cmd_t instance to remove any alises to.
    /// @return true if the alias was removed.
    bool alias_remove(const cmd_t* cmd);

    /// @brief Find a cmd_t instance given its alias name.
    ///
    /// @param alias the string alias to search for an associated cmd_t instance.
    /// @return cmd_t instance linked to this alias otherwise nullptr.
    cmd_t* alias_find(const std::string& alias) const
    {
        auto itt = alias_.find(alias);
        return itt == alias_.end() ? nullptr : itt->second;
    }

protected:
    /// @brief Execute a command expression, calling the relevant cmd_t instance with arguments.
    ///
    /// @param expression string to execute.
    /// @param output output stream that can be written to during execution.
    /// @return true if the command executed successfully.
    bool execute_imp(const std::string& expr, cmd_output_t* output);
};
