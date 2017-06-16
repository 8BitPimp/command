#include <assert.h>
#include <string>
#include <vector>

#include "cmd_expr.h"

namespace {
struct exp_token_t {
    enum type_t {
        e_value,
        e_identifier,
        e_operator,
    };

    type_t type_;

    enum operator_t {
        e_op_add,
        e_op_sub,
        e_op_assign,
        e_op_mul,
        e_op_div,
        e_op_and,
        e_op_or,
        e_op_lparen,
        e_op_rparen,
    };

    union {
        uint64_t value_;
        std::string * ident_;
        operator_t op_;
    };
};

static bool is_operator(const char ch)
{
    switch (ch) {
    case '(':
    case ')':
    case '+':
    case '-':
    case '/':
    case '*':
    case '%':
    case '&':
    case '|':
    case '=':
    case '.':
        return true;
    default:
        return false;
    }
}

static bool is_value(const char ch)
{
    bool ret = false;
    ret |= ch >= 'a' && ch <= 'z';
    ret |= ch >= 'A' && ch <= 'Z';
    ret |= ch >= '0' && ch <= '9';
    ret |= ch == '$';
    ret |= ch == '_';
    return ret;
}

static bool is_alpha(const char ch)
{
    return ch >= '0' && ch <= '9';
}

static bool is_whitespace(const char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static bool is_ident(const std::string& val)
{
    return val.empty() ? false : (!is_alpha(val[0]));
}

static size_t tokenize(const std::string& input, std::deque<std::string>& out)
{
    out.clear();
    const char* h = input.c_str();
    const char* t = input.c_str();
    for (; *h != '\0'; ++h) {
        const char ch = *h;
        // if head == tail
        if (h == t) {
            // skip over whitespaces
            if (is_whitespace(ch)) {
                ++t;
                continue;
            }
            // push operators immediately
            if (is_operator(ch)) {
                out.push_back(std::string(1, ch));
                t = h + 1;
                continue;
            }
        }
        // if head != tail
        else {
            // non value types signal push point
            if (!is_value(ch)) {
                std::string tok = std::string(t, h);
                out.push_back(std::move(tok));
                t = h;
                h -= 1;
            }
        }
    }
    // push any remaining tokens
    if (h != t) {
        out.push_back(std::string{ t, h });
    }
    // return number of parsed tokens
    return out.size();
}
} // namespace {}

struct cmd_expr_imp_t {
    std::vector<std::string> stack_;
    std::deque<std::string> input_;
    std::map<std::string, std::string>& idents_;
    std::string error_;

    cmd_expr_imp_t(std::map<std::string, std::string>& i)
        : idents_(i)
    {
    }

    /* evaluate a given expression */
    bool evaluate(const std::string& exp)
    {
        if (!tokenize(exp, input_)) {
            return false;
        }
        if (!expr(0)) {
            return false;
        }
        return stack_.size() == 1;
    }

protected:
    /* return true if we have a specific token in input stream */
    bool input_found(const std::string& tok)
    {
        if (input_peek() == tok) {
            return input_.pop_front(), true;
        } else {
            return false;
        }
    }

    /* peek the next symbol on the input stack */
    const std::string& input_peek() const
    {
        static std::string none = std::string{};
        return input_.empty() ? none : input_.front();
    }

    /* return next token from input stream */
    bool input_next(std::string& out)
    {
        if (input_.empty()) {
            return false;
        } else {
            out = input_.front();
            input_.pop_front();
            return true;
        }
    }

    /* output a literal or identifier */
    bool parse_ident()
    {
        if (input_.empty()) {
            return false;
        }
        if (input_found("(")) {
            if (!expr(0x0)) {
                return error("error in parenthesis expression");
            }
            if (!input_found(")")) {
                return error("unmatched parenthesis");
            }
        } else {
            std::string temp;
            if (!input_next(temp)) {
                return error("unable to shift next input token");
            }
            stack_.push_back(temp);
        }
        return true;
    }

    /* convert a string to a raw value */
    bool value_get(const std::string& val, uint64_t& out)
    {
        const char* tval = val.c_str();
        if (val.empty()) {
            return false;
        }
        if (!is_alpha(val[0])) {
            // try to parse as an identifier
            auto itt = idents_.find(val);
            if (itt != idents_.end()) {
                // lookup stored value
                tval = itt->second.c_str();
            } else {
                // unknown identifier
                return false;
            }
        }
        // try to parse as an integer literal
        uint64_t v = 0;
        bool neg = false;
        if (!cmd_token_t::strtoll(tval, v, neg)) {
            return error("cant convert '%s' to integer", tval);
        }
        assert(neg == false);
        return out = v, true;
    }

    bool value_to_hex(uint64_t val, std::string& out) const
    {
        static const char* table = "0123456789abcdef";
        char temp[32];
        uint32_t head = 0;
        // hex start marker
        out.clear();
        out.append("0x");
        // hexify
        do {
            const uint32_t digit = val % 16;
            temp[head++] = table[digit];
            val = val / 16;
        } while (val);
        // append in reverse order
        for (; head; --head) {
            out.append(1, temp[head - 1]);
        }
        return true;
    }

    bool value_eval(const std::string& in, std::string& out)
    {
        uint64_t val = 0;
        if (!value_get(in, val)) {
            return error("cant evaluate '%s'", in.c_str());
        }
        return value_to_hex(val, out);
    }

    /* pop a value from the working stack */
    std::string stack_pop()
    {
        if (!stack_.empty()) {
            const std::string out = *stack_.rbegin();
            stack_.pop_back();
            return out;
        } else {
            return std::string{};
        }
    }

    /* push a value onto the working stack (int -> string) */
    bool stack_push(const uint64_t val)
    {
        // convert number to hex
        std::string temp;
        if (!value_to_hex(val, temp)) {
            return error("cant convert '0x%llx' to hex", val);
        } else {
            stack_.push_back(temp);
            return true;
        }
    }

    /* precidence for specific operators */
    uint32_t op_prec(const std::string& op)
    {
        if (op == "(" || op == ")") {
            return 0;
        }
        if (op == "=") {
            return 1;
        }
        if (op == "&" || op == "|") {
            return 2;
        }
        if (op == "-" || op == "+") {
            return 3;
        }
        if (op == "%" || op == "*" || op == "/") {
            return 4;
        }
        /* unknown */ {
            return -1;
        }
    }

    bool op_apply_assign(const std::string& lhs, const std::string& rhs)
    {
        if (!is_ident(lhs)) {
            // error("cant assign to a literal %s", lhs.c_str());
            return false;
        }
        if (is_ident(rhs)) {
            // convert identifier to an rvalue
            std::string rval;
            if (!value_eval(rhs, rval)) {
                return error("cant convert '%s' into rvalue", rhs.c_str());
            }
            idents_[lhs] = rval;
        } else {
            // is already and rvalue literal so use directly
            idents_[lhs] = rhs;
        }
        stack_.push_back(lhs);
        return true;
    }

    bool error(const char* fmt, ...)
    {
        if (error_.empty()) {
            char temp[1024];
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(temp, sizeof(temp), fmt, ap);
            va_end(ap);
            error_.assign(temp);
        }
        return false;
    }

    bool op_apply_generic(const char op, const std::string& lhs, const std::string& rhs)
    {
        uint64_t vlhs = 0, vrhs = 0;
        if (!value_get(lhs, vlhs) || !value_get(rhs, vrhs)) {
            return error("unable to convert lhr or rhs to rvalues");
        }
        switch (op) {
        case '&':
            return stack_push(vlhs & vrhs), true;
        case '|':
            return stack_push(vlhs | vrhs), true;
        case '-':
            return stack_push(vlhs - vrhs), true;
        case '+':
            return stack_push(vlhs + vrhs), true;
        case '*':
            return stack_push(vlhs * vrhs), true;
        case '/':
            return stack_push(vlhs / vrhs), true;
        case '%':
            return stack_push(vlhs & vrhs), true;
        default:
            return error("unknown operator '%c'", op);
        }
    }

    /* apply an operator to the working value stack */
    bool op_apply(std::string& op_str)
    {
        assert(!op_str.empty());
        const std::string rhs = stack_pop();
        const std::string lhs = stack_pop();
        if (lhs.empty() || rhs.empty() || op_str.empty()) {
            return error("expression not fully formed");
        }
        const char op = op_str[0];
        switch (op) {
        case '=':
            return op_apply_assign(lhs, rhs);
        default:
            return op_apply_generic(op, lhs, rhs);
        }
    }

    /* consume input until a minium precedence is reached */
    bool expr(uint32_t min_prec)
    {
        // consume a literal or identifier
        if (!parse_ident()) {
            return error("expecting literal or identifier");
        }
        // check for end of input
        if (input_.empty()) {
            return true;
        }
        std::string op;
        while (op_prec(input_peek()) > min_prec) {
            // get the next operator
            if (!input_next(op)) {
                return error("expecting operator");
            }
            if (!expr(op_prec(op))) {
                return false;
            }
            // apply this operator
            if (!op_apply(op)) {
                return error("unable to apply operator '%s'", op.c_str());
            }
            // if we have exhausted the input stop looping
            if (input_.empty()) {
                break;
            }
        }
        return true;
    }
}; // struct cmd_expr_imp_t

bool cmd_expr_t::cmd_expr_eval_t::on_execute(cmd_tokens_t& tok, cmd_output_t& out)
{
    // turn arguments into expression string
    std::string expr;
    if (!join_expr(tok, expr)) {
        return error(out, "  malformed expression");
    }
    // execute the expression
    cmd_expr_imp_t state(expr_.idents_);
    if (!state.evaluate(expr)) {
        out.println("  error: %s", state.error_.c_str());
        return false;
    }
    // print results
    for (const std::string& val : state.stack_) {
        out.print("  ");
        if (is_ident(val)) {
            auto& idents = state.idents_;
            auto itt = idents.find(val);
            if (itt == idents.end()) {
                // unknown identifier
                out.println("unknown identifier '%s'", val.c_str());
            } else {
                // print key value pair
                const std::string& key = itt->first;
                const std::string& value = itt->second;
                out.println("  %s = %s", key.c_str(), value.c_str());
            }
        } else {
            out.println("  %s", val.c_str());
        }
    }
    return true;
}
