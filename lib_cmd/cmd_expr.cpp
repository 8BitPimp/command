#include <assert.h>
#include <string>
#include <vector>
#include <array>

#include "cmd_expr.h"

struct cmd_exp_error_t {

    std::vector<std::string> error_;

    bool error(const char* fmt, ...)
    {
        if (error_.empty()) {
            std::array<char, 1024> temp;
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(temp.data(), temp.size(), fmt, ap);
            va_end(ap);
            error_.push_back(temp.data());
        }
        /* return false to make error prop easier */
        return false;
    }

    bool print(cmd_output_t& out)
    {
        for (const std::string& err : error_) {
            out.println("  %s", err.c_str());
        }
        return true;
    }

    bool error_non_single_result()
    {
        return error("expression did not produce single result");
    }

    bool error_in_paren_exp()
    {
        return error("error in parenthesis expression");
    }

    bool error_unmatched_paren()
    {
        return error("unmatched parenthesis");
    }

    bool error_cant_shift_input()
    {
        return error("unable to shift next input token");
    }

    bool error_cant_deref(const char* ident)
    {
        return error("cant dereference '%s'", ident);
    }

    bool error_cant_assign_literal()
    {
        return error("cant assign to a literal");
    }

    bool error_malformed_expr()
    {
        return error("malformed expression");
    }

    bool error_div_zero()
    {
        return error("divide by zero");
    }

    bool error_unknown_op(const char op)
    {
        return error("unknown operator '%c'", op);
    }

    bool error_missing_rhs()
    {
        return error("missing rhs of expression");
    }

    bool error_missing_lhs()
    {
        return error("missing lhs of expression");
    }

    bool error_rhs_needs_rvalue()
    {
        return error("rhs must be an rvalue");
    }

    bool error_expect_op()
    {
        return error("expecting operator");
    }

    bool error_expect_lit_or_ident()
    {
        return error("expecting literal or identifier");
    }

    bool error_applying_op(const char op)
    {
        return error("unable to apply operator '%c'", op);
    }
};

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

struct cmd_exp_lexer_t {
    bool is_operator(const char ch)
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

    bool is_operator(const std::string& in)
    {
        if (in.empty()) {
            return false;
        }
        const char ch = in[0];
        return is_operator(ch);
    }

    bool is_value(const char ch)
    {
        bool ret = false;
        ret |= ch >= 'a' && ch <= 'z';
        ret |= ch >= 'A' && ch <= 'Z';
        ret |= ch >= '0' && ch <= '9';
        ret |= ch == '$';
        ret |= ch == '_';
        return ret;
    }

    bool is_alpha(const std::string& val)
    {
        if (val.empty()) {
            return false;
        } else {
            const char ch = val[0];
            return ch >= '0' && ch <= '9';
        }
    }

    bool is_whitespace(const char ch)
    {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    }

    bool is_ident(const std::string& val)
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

    // clasify and push item into input token queue
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
            cmd_util_t::strtoll(item.c_str(), tok.value_, neg);
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

    // produce parsed token queue from an input string
    bool tokenize(const std::string& input, std::deque<exp_token_t>& out)
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
};

} // namespace {}

// command expression evaluation implementation
struct cmd_expr_imp_t {
    std::vector<exp_token_t> stack_;
    std::deque<exp_token_t> input_;
    std::map<std::string, uint64_t>& idents_;
    cmd_exp_error_t error_;

    cmd_expr_imp_t(std::map<std::string, uint64_t>& i)
        : idents_(i)
    {
    }

    /* evaluate a given expression */
    bool evaluate(const std::string& exp)
    {
        cmd_exp_lexer_t lexer;
        if (!lexer.tokenize(exp, input_)) {
            return false;
        }
        if (!expr(0)) {
            return false;
        }
        if (stack_.size() != 1) {
            return error_.error_non_single_result();
        }
        return true;
    }

protected:
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

    /* end of file */
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
                return error_.error_in_paren_exp();
            }
            if (!input_found_op(')')) {
                return error_.error_unmatched_paren();
            }
        } else {
            exp_token_t temp;
            if (!input_next(temp)) {
                return error_.error_cant_shift_input();
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

    /* lookup idenfier value */
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
            return error_.error_cant_assign_literal();
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
                return error_.error_cant_deref(lhs.ident_.c_str());
            }
        }
        if (lhs.type_ != exp_token_t::e_value || rhs.type_ != exp_token_t::e_value) {
            return error_.error_malformed_expr();
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
                return error_.error_div_zero();
            }
            return stack_push(lhs.value_ / rhs.value_), true;
        case '%':
            if (rhs.value_ == 0) {
                return error_.error_div_zero();
            }
            return stack_push(lhs.value_ % rhs.value_), true;
        default:
            return error_.error_unknown_op(op.op_);
        }
    }

    /* apply an operator to the working value stack */
    bool op_apply(const exp_token_t& op)
    {
        assert(op.type_ == exp_token_t::e_operator);
        exp_token_t rhs, lhs;
        if (!stack_pop(rhs)) {
            return error_.error_missing_rhs();
        }
        if (!stack_pop(lhs)) {
            return error_.error_missing_lhs();
        }
        // we can dereference the RHS in anticipation
        if (rhs.type_ == exp_token_t::e_identifier) {
            if (!dereference(rhs, rhs)) {
                return error_.error_cant_deref(rhs.ident_.c_str());
            }
        }
        if (rhs.type_ != rhs.e_value) {
            return error_.error_rhs_needs_rvalue();
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
            return error_.error_expect_lit_or_ident();
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
                    return error_.error_expect_op();
                }
                if (op_prec(temp) <= min_prec) {
                    break;
                }
            }
            // get the next operator
            if (!input_next(op)) {
                return error_.error_expect_op();
            }
            if (op.type_ != op.e_operator) {
                return error_.error_expect_op();
            }
            if (!expr(op_prec(op))) {
                return false;
            }
            // apply this operator
            if (!op_apply(op)) {
                return error_.error_applying_op(op.op_);
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
    auto indent = out.indent(2);
    // turn arguments into expression string
    std::string expr;
    if (!join_expr(tok, expr)) {
        return cmd_locale_t::malformed_exp(out), false;
    }
    // execute the expression
    cmd_expr_imp_t state(parser_.idents_);
    if (!state.evaluate(expr)) {
        return state.error_.print(out), false;
    }
    indent.add(2);
    // print results
    for (const exp_token_t& val : state.stack_) {
        switch (val.type_) {
        case exp_token_t::e_identifier: {
            auto& idents = state.idents_;
            auto itt = idents.find(val.ident_);
            if (itt == idents.end()) {
                // unknown identifier
                cmd_locale_t::unknown_ident(out, val.ident_.c_str());
            } else {
                // print key value pair
                const std::string& key = itt->first;
                const uint64_t& value = itt->second;
                out.println("%s = 0x%llx", key.c_str(), value);
            }
            break;
        }
        case exp_token_t::e_value:
            out.println("0x%llx", val.value_);
            break;
        default:
            return cmd_locale_t::not_val_or_ident(out), false;
        }
    }
    return true;
}
