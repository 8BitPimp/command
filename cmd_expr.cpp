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
        e_eof
    };
    type_t type_;

    enum operator_t {
        e_op_add = '+',
        e_op_sub = '-',
        e_op_mul = '*',
        e_op_div = '/',
        e_op_and = '&',
        e_op_or = '|',
        e_op_assign = '=',
        e_op_lparen = '(',
        e_op_rparen = ')',
    };

    std::string ident_;
    union {
        uint64_t value_;
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

static bool is_operator(const std::string& in)
{
    if (in.empty()) {
        return false;
    }
    const char ch = in[0];
    return is_operator(ch);
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

static bool is_alpha(const std::string& val)
{
    if (val.empty()) {
        return false;
    } else {
        const char ch = val[0];
        return ch >= '0' && ch <= '9';
    }
}

static bool is_whitespace(const char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static bool is_ident(const std::string& val)
{
    if (val.empty()) {
        return false;
    } else {
        const char ch = val[0];
        bool ret = false;
        ret |= ch >= 'a' && ch <= 'z';
        ret |= ch >= 'A' && ch <= 'Z';
        ret |= ch == '_';
        return ret;
    }
}

bool cmd_strtoll(const char* in, uint64_t& out, bool& neg)
{
    neg = false;
    if (*in == '-') {
        neg = true;
        ++in;
    }
    uint32_t base = 10;
    if (memcmp(in, "0x", 2) == 0) {
        base = 16;
        in += 2;
    }
    uint64_t accum = 0;
    for (; *in != '\0'; ++in) {
        accum *= base;
        const uint8_t ch = *in;
        if (ch >= '0' && ch <= '9') {
            accum += ch - '0';
        } else if (base == 16) {
            if (ch >= 'a' && ch <= 'f') {
                accum += (ch - 'a') + 10;
            } else if (ch >= 'A' && ch <= 'F') {
                accum += (ch - 'A') + 10;
            }
        } else {
            return (*in == ' ');
        }
    }
    return out = accum, true;
}

bool push_item(std::deque<exp_token_t>& q, const std::string& item)
{
    if (item.empty()) {
        return false;
    }
    exp_token_t tok;
    memset(&tok, 0, sizeof(tok));
    if (is_alpha(item)) {
        tok.type_ = tok.e_value;
        bool neg = false;
        cmd_strtoll(item.c_str(), tok.value_, neg);
    } else if (is_ident(item)) {
        tok.type_ = tok.e_identifier;
        tok.ident_ = item;
    } else if (is_operator(item)) {
        tok.type_ = tok.e_operator;
        const char ch = item[0];
        tok.op_ = (exp_token_t::operator_t)ch;
    } else {
        return false;
    }
    q.push_back(tok);
    return true;
}

static bool tokenize(const std::string& input, std::deque<exp_token_t>& out)
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
                if (!push_item(out, std::string(1, ch))) {
                    return false;
                }
                t = h + 1;
                continue;
            }
        }
        // if head != tail
        else {
            // non value types signal push point
            if (!is_value(ch)) {
                if (!push_item(out, std::string(t, h))) {
                    return false;
                }
                t = h;
                h -= 1;
            }
        }
    }
    // push any remaining tokens
    if (h != t) {
        if (!push_item(out, std::string(t, h))) {
            return false;
        }
    }
    // push end of file token
    out.push_back(exp_token_t{ exp_token_t::e_eof });
    // return number of parsed tokens
    return 0 != out.size();
}
} // namespace {}

struct cmd_expr_imp_t {
    std::vector<exp_token_t> stack_;
    std::deque<exp_token_t> input_;
    std::map<std::string, uint64_t>& idents_;
    std::string error_;

    cmd_expr_imp_t(std::map<std::string, uint64_t>& i)
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
        if (stack_.size() != 1) {
            return error("expression did not produce single result");
        }
        return true;
    }

protected:
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

    bool input_found_op(const char op)
    {
        assert(!input_.empty());
        const exp_token_t& tok = input_.front();
        if (tok.type_ != exp_token_t::e_operator) {
            return false;
        }
        if (tok.op_ == op) {
            input_.pop_front();
            return true;
        }
        return false;
    }

    /* peek the next symbol on the input stack */
    const exp_token_t& input_peek() const
    {
        assert(!input_.empty());
        return input_.front();
    }

    /* return next token from input stream */
    bool input_next(exp_token_t& out)
    {
        assert(!input_.empty());
        out = input_.front();
        if (out.type_ != exp_token_t::e_eof) {
            input_.pop_front();
        }
        return true;
    }

    bool input_eof() const
    {
        assert(!input_.empty());
        return input_.front().type_ == exp_token_t::e_eof;
    }

    /* output a literal or identifier */
    bool parse_ident()
    {
        assert(!input_.empty());
        if (input_found_op('(')) {
            if (!expr(0x0)) {
                return error("error in parenthesis expression");
            }
            if (!input_found_op(')')) {
                return error("unmatched parenthesis");
            }
        } else {
            exp_token_t temp;
            if (!input_next(temp)) {
                return error("unable to shift next input token");
            }
            stack_.push_back(temp);
        }
        return true;
    }

    /* pop a value from the working stack */
    bool stack_pop(exp_token_t& out)
    {
        if (!stack_.empty()) {
            out = *stack_.rbegin();
            stack_.pop_back();
            return true;
        } else {
            return false;
        }
    }

    /* push a value onto the working stack (int -> string) */
    bool stack_push(const uint64_t val)
    {
        // convert number to hex
        exp_token_t temp;
        temp.type_ = temp.e_value;
        temp.value_ = val;
        stack_.push_back(temp);
        return true;
    }

    /* precidence for specific operators */
    uint32_t op_prec(const exp_token_t& in) const
    {
        assert(in.type_ == exp_token_t::e_operator);
        const char op = in.op_;
        if (op == '(' || op == ')') {
            return 0;
        }
        if (op == '=') {
            return 1;
        }
        if (op == '&' || op == '|') {
            return 2;
        }
        if (op == '-' || op == '+') {
            return 3;
        }
        if (op == '%' || op == '*' || op == '/') {
            return 4;
        }
        /* unknown */ {
            return -1;
        }
    }

    bool dereference(const exp_token_t& in, exp_token_t& out) const
    {
        // note: in and out may reference the same object
        if (in.type_ == in.e_value) {
            out.type_ = out.e_value;
            out.value_ = in.value_;
            return true;
        }
        if (in.type_ == in.e_identifier) {
            auto itt = idents_.find(in.ident_);
            if (itt == idents_.end()) {
                return false;
            }
            out.type_ = out.e_value;
            out.value_ = itt->second;
            return true;
        }
        return false;
    }

    bool op_apply_assign(const exp_token_t& lhs, exp_token_t rhs)
    {
        if (lhs.type_ != exp_token_t::e_identifier) {
            return error("cant assign to a literal");
        }
        assert(rhs.type_ == exp_token_t::e_value);
        idents_[lhs.ident_] = rhs.value_;
        stack_.push_back(lhs);
        return true;
    }

    bool op_apply_generic(const exp_token_t& op, exp_token_t& lhs, const exp_token_t& rhs)
    {
        assert(op.type_ == exp_token_t::e_operator);
        if (lhs.type_ == exp_token_t::e_identifier) {
            if (!dereference(lhs, lhs)) {
                return error("cant dereference '%s'", lhs.ident_.c_str());
            }
        }
        if (lhs.type_ != exp_token_t::e_value || rhs.type_ != exp_token_t::e_value) {
            return error("malformed expression");
        }
        switch (op.op_) {
        case '&':
            return stack_push(lhs.value_ & rhs.value_), true;
        case '|':
            return stack_push(lhs.value_ | rhs.value_), true;
        case '-':
            return stack_push(lhs.value_ - rhs.value_), true;
        case '+':
            return stack_push(lhs.value_ + rhs.value_), true;
        case '*':
            return stack_push(lhs.value_ * rhs.value_), true;
        case '/':
            if (rhs.value_ == 0) {
                return error("divide by zero");
            }
            return stack_push(lhs.value_ / rhs.value_), true;
        case '%':
            if (rhs.value_ == 0) {
                return error("divide by zero");
            }
            return stack_push(lhs.value_ % rhs.value_), true;
        default:
            return error("unknown operator '%c'", op);
        }
    }

    /* apply an operator to the working value stack */
    bool op_apply(const exp_token_t& op)
    {
        assert(op.type_ == exp_token_t::e_operator);
        exp_token_t rhs, lhs;
        if (!stack_pop(rhs)) {
            return error("missing rhs of expression");
        }
        if (!stack_pop(lhs)) {
            return error("missing lhs of expression");
        }
        // we can dereference the RHS in anticipation
        if (rhs.type_ == exp_token_t::e_identifier) {
            if (!dereference(rhs, rhs)) {
                return error("cant dereference '%s'", rhs.ident_.c_str());
            }
        }
        if (rhs.type_ != rhs.e_value) {
            return error("rhs must be an rvalue");
        }
        // dispatch based on operator type
        switch (op.op_) {
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
        if (input_eof()) {
            return true;
        }
        exp_token_t op;
        // todo: make this peek robust and check if operator
        while (true) {
            // halting condition
            {
                const exp_token_t& temp = input_peek();
                if (temp.type_ != temp.e_operator) {
                    return error("expecting operator");
                }
                if (op_prec(temp) <= min_prec) {
                    break;
                }
            }
            // get the next operator
            if (!input_next(op)) {
                return error("expecting operator");
            }
            if (op.type_ != op.e_operator) {
                return error("expecting operator");
            }
            if (!expr(op_prec(op))) {
                return false;
            }
            // apply this operator
            if (!op_apply(op)) {
                return error("unable to apply operator '%c'", op.op_);
            }
            // if we have exhausted the input stop looping
            if (input_eof()) {
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
    cmd_expr_imp_t state(parser_.idents_);
    if (!state.evaluate(expr)) {
        out.println("  error: %s", state.error_.c_str());
        return false;
    }
    // print results
    for (const exp_token_t& val : state.stack_) {
        out.print("  ");
        switch (val.type_) {
        case exp_token_t::e_identifier: {
            auto& idents = state.idents_;
            auto itt = idents.find(val.ident_);
            if (itt == idents.end()) {
                // unknown identifier
                out.println("unknown identifier '%s'", val.ident_.c_str());
            } else {
                // print key value pair
                const std::string& key = itt->first;
                const uint64_t& value = itt->second;
                out.println("  %s = 0x%llx", key.c_str(), value);
            }
            break;
        }
        case exp_token_t::e_value:
            out.println("  0x%llx", val.value_);
            break;
        default:
            out.println("  return type not value or identifier");
            return false;
        }
    }
    return true;
}
