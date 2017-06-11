#include <cassert>
#include <limits.h>

#include "cmd.h"

namespace {
// substring match (return match length, or -1 if different)
static int32_t str_match(const char* str,
    const char* sub)
{
    assert(str && sub);
    for (int32_t i = 0;; ++i) {
        // end of source string
        if (str[i] == '\0') {
            if (str[i] == sub[i]) {
                // perfect match
                return INT_MAX;
            } else {
                // substring too long
                return -1;
            }
        }
        // end of sub string
        if (sub[i] == '\0') {
            return i;
        }
        // string differ
        if (str[i] != sub[i]) {
            return -1;
        }
    }
}

// find a list of matching commands
static bool find_matches(cmd_list_t& list,
    const char* sub,
    std::vector<cmd_t*>& vec)
{
    assert(sub);
    int32_t score = 0;
    for (auto& item : list) {
        int32_t val = str_match(item->name_, sub);
        if (val > score) {
            vec.clear();
            vec.push_back(item.get());
            score = val;
        } else if (val == score) {
            vec.push_back(item.get());
        } else {
            // drop on the floor
        }
    }
    return vec.size() > 0;
}

// tokenize the input string
static size_t tokenize(const char* in,
    cmd_tokens_t& out)
{
    assert(in);
    const char* src = in;
    const char* start = src;
    // step over the string
    while (*src) {
        // find for next white space
        if (*src == ' ') {
            // extract this token
            out.push(std::string(start, src));
            // skip trailing white space
            for (; *src == ' '; ++src)
                ;
            start = src;
        } else {
            ++src;
        }
    }
    // skip any trailing tokens
    if (src != start) {
        // extract this token
        out.push(std::string(start, src));
    }
    // return number of tokens
    return out.size();
}
} // namespace

int cmd_levenshtein(const char* s1, const char* s2)
{
#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
    uint32_t x, y, lastdiag, olddiag;
    const size_t s1len = strlen(s1);
    const size_t s2len = strlen(s2);
    uint32_t* column = (uint32_t*)alloca((s1len + 1) * sizeof(uint32_t));
    for (y = 1; y <= s1len; y++)
        column[y] = y;
    for (x = 1; x <= s2len; x++) {
        column[0] = x;
        for (y = 1, lastdiag = x - 1; y <= s1len; y++) {
            olddiag = column[y];
            column[y] = MIN3(column[y] + 1,
                column[y - 1] + 1,
                lastdiag + (s1[y - 1] == s2[x - 1] ? 0 : 1));
            lastdiag = olddiag;
        }
    }
    return column[s1len];
#undef MIN3
}

bool cmd_parser_t::execute(const std::string& expr, cmd_output_t& out)
{
    const std::string prev_cmd = last_cmd();

    // add to history buffer
    history_.push_back(expr);
    // tokenize command string
    cmd_tokens_t tokens;
    if (tokenize(expr.c_str(), tokens) == 0) {
        if (!last_cmd().empty()) {
            return execute(prev_cmd, out);
        } else {
            // no commands entered
            return false;
        }
    }

    cmd_list_t* list = &sub_;
    std::vector<cmd_t*> cmd_vec;

    // check for aliases
    cmd_t* cmd = alias_find(tokens.front());
    if (cmd) {
        tokens.pop();
    } else {
        while (!tokens.empty()) {
            // find best matching sub command
            cmd_vec.clear();
            find_matches(*list, tokens.front().c_str(), cmd_vec);
            if (cmd_vec.size() == 0) {
                // no sub commands to match
                break;
            } else if (cmd_vec.size() == 1) {
                cmd = cmd_vec.front();
                list = &cmd->sub_;
                // remove front item
                tokens.pop();
            } else {
                // ambiguous matches
                cmd = nullptr;
                out.println("  possible completions:");
                for (auto c : cmd_vec) {
                    out.println("    %s", c->name_);
                }
                break;
            }
        }
    }
    // execute command
    return cmd ? cmd->on_execute(tokens, out) : false;
}

bool cmd_parser_t::find(const std::string& expr,
    std::vector<std::string>& out)
{
    (void)expr;
    (void)out;
    return false;
}

bool cmd_parser_t::alias_add(cmd_t* cmd, const std::string& alias)
{
    assert(cmd && !alias.empty());
    alias_[alias] = cmd;
    return true;
}

bool cmd_parser_t::alias_remove(const std::string& alias)
{
    auto itt = alias_.find(alias);
    if (itt != alias_.end()) {
        alias_.erase(itt);
        return true;
    } else {
        return false;
    }
}

bool cmd_parser_t::alias_remove(const cmd_t* cmd)
{
    for (auto itt = alias_.begin(); itt != alias_.end();) {
        assert(itt->second);
        if (itt->second == cmd) {
            itt = alias_.erase(itt);
        } else {
            ++itt;
        }
    }
    return true;
}