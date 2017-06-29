----
# REPL command line parser

A simple but powerfull command line REPL.


****
```c
typedef std::vector<std::unique_ptr<struct cmd_t>> cmd_list_t
```

cmd_list_t, list of cmd_t instances.



****
```c
typedef std::map<std::string, uint64_t> cmd_idents_t
```

cmd_idents_t, identfier list used for cmd_tokens_t substitutions.



****
```c
typedef void* cmd_baton_t
```

cmd_baton_t, baton used for passing user data to cm_t instances.



----
## struct cmd_util_t

cmd_util_t, utility functions for the command parser.



****
```c
static bool strtoll(const char* in, uint64_t& out, bool& neg)
```

convert string to 64 bit integer.

Params:
- in input string to parse as integer
- out output integer to receive contersion
- neg output boolean to be set if integer should be negated

Return:
- true if the conversion was successfull



****
```c
static uint32_t levenshtein(const char* a, const char* b)
```

levenshtein string distance function.

Params:
- s1 input string a
- s2 input string b

Return:
- edit distance between strings s1 and s2



****
```c
static int32_t str_match(const char* str, const char* sub)
```

partial substring match.

Return:
- number of characters between str and sub that match or -1 if different



----
## struct cmd_output_t

cmd_output_t, command output interface base class.

this class brokers all output text writing from cmd_t classes during command
execution. using a specific class we can ensure consisten output and eliminate
output race conditions. a versatile iterface also makes it easy to output text
in a range of formats. cmd_output_t is also a factory for derived classes with
specialisations.


****
```c
static cmd_output_t* create_output_stdio(FILE* fd)
```

Create a cmd_output_t instance that will write directly to a file descriptor.

Params:
- fd, the file descriptior that all output will be written to.

Return:
- cmd_output_t instance.



----
## struct indent_t

indent_t, indent control helper class.



----
## struct guard_t

guard_t, output mutex helper class.



****
```c
guard_t guard()
```

obtain scope guard for the output mutex.

Return:
- scope guard object for the output mutex.



****
```c
cmd_output_t()
```

constructor.


****
```c
virtual ~cmd_output_t() {}
```

virtual destructor.


****
```c
virtual void lock() = 0
```

aquire the output mutex.


****
```c
virtual void unlock() = 0
```

release the output mutex.


****
```c
indent_t indent(uint32_t next = 2)
```

push a new indentation level.

Return:
- indent helper class.



****
```c
template <bool INDENT = true>
void print(const char* fmt, ...)
```

print a format string into this output stream.

Params:
- INDENT follow indentation marker from output string.
- fmt format string.
- variable length argument list.

print to output string without appending a new line.


****
```c
template <bool INDENT = true>
void println(const char* fmt, ...)
```

print a format string into this output stream.

Params:
- INDENT follow indentation marker from output string.
- fmt format string.
- variable length argument list.

print to output string and append a new line.


****
```c
virtual void eol() = 0
```

Append an end of line character.


****
```c
uint32_t indent_
```

Current indentation level.


----
## struct cmd_locale_t

cmd_locale_t, command locale text definitions.



----
## struct cmd_token_t

cmd_token_t, command arguement token.

User input is processed, it is parsed to form a list of tokens.  These tokens
are encapsulated as a cmd_token_t.  A list of tokens is defined as the
cmd_tokens_t type.  The cmd_token_t type makes it easier to convert string
tokens between multiple data types.


****
```c
cmd_token_t() = default
```

constructor.


****
```c
cmd_token_t(const std::string& string)
```

cmd_token_t constructor.

Params:
- string token.



****
```c
const std::string& get() const
```

return token as a string.

Return:
- underlying token string.



****
```c
template <typename type_t>
bool get(type_t& out) const
```

get token as an integer.

Params:
- output integer to store the conversion.

Return:
- true if the token represents an integer.



****
```c
bool operator==(const cmd_token_t& rhs) const
```

test for token equality.

Params:
- rhs token to check equality with.

Return:
- true if tokens are equal.



****
```c
template <typename type_t>
bool operator==(const type_t& rhs) const
```

test for token equality.

Params:
- rhs token to check equality with.

Return:
- true if token is equal to rhs.



****
```c
operator std::string() const
```

std::string cast operator.

Return:
- token as string.



****
```c
const char* c_str() const
```

c string cast operator.

Return:
- c string representataion of token.



----
## struct cmd_tokens_t

cmd_tokens_t, command arguments token list.



****
```c
bool get(const std::string& name) const
```

check if a flag was passed to the token list.

Return:
- true if 'name' flag was passed as an argument.



****
```c
bool empty() const
```

check if the flag set is empty.

Return:
- true if the flags set is empty, otherwise false.



****
```c
std::set<std::string> flags_
```

command token flags.


****
```c
bool get(const std::string& name, cmd_token_t& out) const
```

retreive the argument to a passed token pair.

Return:
- true if the pair was in the token list.



****
```c
bool empty() const
```

check if the pairs map is empty.

Return:
- true if the pairs set is empty, otherwise false.



****
```c
std::map<std::string, cmd_token_t> pairs_
```

key value pair arguments.


****
```c
size_t size() const
```

return number of tokens in the token list.

Return:
- number of tokens in the token list.



****
```c
bool get(std::string& out)
```

get the front most token from the token list.

Return:
- true if the front most token could be retrived.



****
```c
bool get(cmd_token_t& out)
```

get the front most token from the token list.

Params:
- output to receive front most token.

Return:
- true if the front most token could be retrived.



****
```c
bool get(uint64_t& out)
```

get the front most token from the token list as an integer.

Params:
- output to receive the front most token.

Return:
- true if the front most token could be interpreted as integer.



****
```c
bool empty() const
```

check if the token list is empty.

Return:
- true if the token list is empty.



****
```c
const cmd_token_t& front() const
```

return a reference to the first token in the queue.

Return:
- reference to first token.



****
```c
const cmd_token_t& back() const
```

return a reference to the last token in the queue.

Return:
- reference to last token.



****
```c
bool pop()
```

pop front most argument from token list.

Return:
- true if front argument was popped.



****
```c
const bool find(const std::string& in) const
```

Find a matching token in the token list.

Params:
- in input string to try and locate in the token list.



****
```c
std::deque<cmd_token_t>& operator()()
```

Accessor for the tokens deque.


****
```c
std::deque<cmd_token_t> tokens_
```

basic token arguments.


****
```c
std::deque<cmd_token_t> raw_
```

raw tokens.


****
```c
cmd_tokens_t(cmd_idents_t* idents)
```

constructor.

Params:
- idents list of identifiers to substitute tokens with.



****
```c
size_t tokenize(const char* in)
```

tokenize and input stream into a cmd_tokens_t instance.

Params:
- in input stream to tokenize.
- out output cmd_tokens_t instance to populate.

Return:
- number of tokens parsed.



****
```c
void push(std::string input)
```

push a new token into this token list.

Params:
- string token to push onto list.



****
```c
cmd_idents_t* idents_
```

list of identifiers that can be substituted for tokens.


****
```c
std::pair<std::string, cmd_token_t> stage_pair_
```

staging area for pairs.


----
## struct cmd_t

cmd_t, the command base class.

this is the base command class that should be extended to handle custom
commands. a cmd_t can be a child of another command or inserted into a
cmd_parser_t as a root command. two key methods can be overriden, on_execute and
on_usage which the cmd_parser_t will invoke based on user input.


****
```c
const char* const name_
```

command name.


****
```c
struct cmd_parser_t& parser_
```

owning command parser.


****
```c
cmd_baton_t user_
```

opaque user data for this command.


****
```c
cmd_t* parent_
```

the parent if this is a child command.


****
```c
cmd_list_t sub_
```

list of child commands.


****
```c
const char* usage_
```

command argument string.


****
```c
const char* desc_
```

command description string.


****
```c
cmd_t(const char* name,
cmd_parser_t& parser,
cmd_t* parent,
cmd_baton_t user = nullptr)
```

cmd_t constructor.

Params:
- const char* name, the name of this command.
- cmd_t* parent, the parent cmd_t instance.
- cmd_baton_t user, the opaque user data passed to this cmd_t instance.



****
```c
template <typename type_t>
type_t* add_sub_command()
```

Add child command to this command.

Return:
- the new cmd_t instance.

instanciate and attach a new child command to this parent command supplying this
commands user_ data during its construction.


****
```c
template <typename type_t>
type_t* add_sub_command(cmd_baton_t user)
```

Add child command to this command.

Params:
- user opaque user data to pass to the new child cmd_t.

Return:
- the new cmd_t instance.

instanciate and attach a new child command to this parent command.


****
```c
virtual bool on_execute(cmd_tokens_t& tok, cmd_output_t& out)
```

Command execution handler.

Params:
- tok token list of arguments supplied by the user.
- out text output stream for writing results to.

Return:
- true if the command executed successfully.

Virtual function that will be called when the user specifies is full path or an
alias to this command. If this command is ternimal then any following input
tokens will be passed to this handler. The caller provided output stream is also
passed to the called command execution handler.


****
```c
void get_command_path(std::string& out) const
```

Return string with hierarchy of parent commands.

Params:
- out the string to store output hierarchy.



****
```c
virtual bool on_usage(cmd_output_t& out) const
```

Print command usage information to output stream.

Params:
- out output stream to print to

Return:
- true if usage was written successfully



****
```c
bool alias_add(const std::string& name)
```

Add an alias for this command.

Params:
- name the string name of the alias to add.

Return:
- true if the alias was associated successfully.

bind a cmt_t instance to a single string token known as an alias. during command
interpretation if the first token matches any alises then the associated cmd_t
instance will be executed.


****
```c
bool error(cmd_output_t& out, const char* fmt, ...)
```

Report an error condition to the output stream.

Params:
- out output stream.
- fmt input format string.
- ... variable args.

Return:
- false so it can be propagated via return statements easily.



****
```c
void print_cmd_list(const cmd_list_t& list, cmd_output_t& out) const
```

Print a list of command names to an output stream.

Params:
- list input command list.
- out output stream to write command names to.



****
```c
void print_sub_commands(cmd_output_t& out) const
```

Print a list of child commands to the output stream.

Params:
- out output stream to write command names to.



----
## struct cmd_parser_t

cmd_parser_t, the command parser.

this type is the main workhorse of the command library.  it forms the root of
the command hieararchy it stores some state that can be accessed via all
commands (alias_, idents_) cmd_parser_t is responsible for parsing and
dispatching user input to the appropriate command


****
```c
cmd_parser_t(cmd_baton_t user = nullptr)
```

cmd_parser_t constructor.

Params:
- user opaque user data pointer passed from parent to child.

Return:
- user a global custom data pointer to be passed to any sub commands.



****
```c
const std::string& last_cmd()
```

Get a string with the last user input to be executed.

Return:
- reference to the last



****
```c
template <typename type_t>
type_t* add_command()
```

Add a new root command to the command parser.

Params:
- type_t class derived from cmd_t base.

Return:
- instance of the newly created command.

add a new root command to the command interpreter. the new subcommand instance
will be passed the global cmd_parser_t user_ data


****
```c
template <typename type_t>
type_t* add_command(cmd_baton_t user)
```

Add a new root command to the command parser.

Params:
- user the user data type to pass to the sub command.

Return:
- instance of the newly created command.



****
```c
cmd_t* add_command(cmd_t*& command)
```

Add a new root command to the command parser.

Params:
- command a command instance to add to the parser

Return:
- the new command instance after moving the parameter

Note: the command parameter will be moved so that ownership is transfered to the
command parser.  command should not be accessed after calling this function and
instead, the return instance should be used in its place.


****
```c
bool execute(const std::string& expr, cmd_output_t* output)
```

Execute expressions, calling the relevant cmd_t instances with arguments.

Params:
- a list of ';' delimited expression strings to execute.
- output output stream that can be written to during execution.

Return:
- true if the command executed successfully.



****
```c
bool alias_add(cmd_t* cmd, const std::string& alias)
```

Add a new parser alias for a cmd_t instance.

Params:
- cmd command instance for which to make an alias.
- alias name for the alias.

Return:
- true if that alias was added.



****
```c
bool alias_remove(const std::string& alias)
```

Remove a previously registered command alias byt name.

Params:
- alias the string command alias to remove.

Return:
- true if the alias was removed.



****
```c
bool alias_remove(const cmd_t* cmd)
```

Remove any previously registered command alises by target cmd_t.

Params:
- cmd the cmd_t instance to remove any alises to.

Return:
- true if the alias was removed.



****
```c
cmd_t* alias_find(const std::string& alias) const
```

Find a cmd_t instance given its alias name.

Params:
- alias the string alias to search for an associated cmd_t instance.

Return:
- cmd_t instance linked to this alias otherwise nullptr.



****
```c
bool execute_imp(const std::string& expr, cmd_output_t* output)
```

Execute a command expression, calling the relevant cmd_t instance with arguments.

Params:
- expression string to execute.
- output output stream that can be written to during execution.

Return:
- true if the command executed successfully.



