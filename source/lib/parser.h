#pragma once
#include <vector>
#include <array>

#include "ccml.h"
#include "token.h"


struct identifier_t {

  identifier_t()
    : offset(0)
    , count(0)
    , token(nullptr)
  {
  }

  identifier_t(const token_t *t, int32_t o, int32_t c=1)
    : offset(o)
    , count(c)
    , token(t) {
  }

  bool is_array() const {
    return count > 1;
  }

  // offset from frame pointer
  int32_t offset;
  // number of items (> 1 == array)
  int32_t count;
  // identifier token
  const token_t *token;
};

struct scope_list_t {

  scope_list_t() : max_depth_(0) {
    reset();
  }

  void enter() {
    const int32_t back = head();
    var_head_.emplace_back(back);
  }

  void leave() {
    var_head_.pop_back();
  }

  void var_add(const token_t &tok, uint32_t count) {
    assert(head() < int32_t(vars_.size()));
    const int32_t offset = var_count();
    vars_[head()++] = identifier_t(&tok, offset, count);
    // update max depth
    max_depth_ = std::max(max_depth_, var_count());
  }

  // return space used for variables in stack frame
  int32_t var_count() const {
    int32_t count = 0;
    for (int i = 0; i < head(); ++i) {
      count += vars_[i].count;
    }
    return count;
  }

  void arg_add(const token_t *tok) {
    identifier_t ident(tok, 0);
    args_[arg_head_++] = ident;
  }

  int32_t arg_count() const {
    return arg_head_;
  }

  // calculate argument offsets from frame pointer
  // note: dont push any more arguments
  void arg_calc_offsets() {
    for (int32_t i = 0; i < arg_head_; ++i) {
      identifier_t &arg = args_[i];
      arg.offset = i - arg_head_;
    }
  }

  const identifier_t *find_ident(const token_t &name) const {
    // note: back to front search could enable shadowing
    // search for locals
    for (int32_t i = head()-1; i >= 0; --i) {
      if (vars_[i].token->str_ == name.str_) {
        return &vars_[i];
      }
    }
    // search for arguments
    for (int32_t i = 0; i < arg_head_; ++i) {
      if (args_[i].token->str_ == name.str_) {
        return &args_[i];
      }
    }
    // none found
    return nullptr;
  }

  void reset() {
    // reset vars
    max_depth_ = 0;
    var_head_.clear();
    var_head_.emplace_back(0);
    // reset args
    arg_head_ = 0;
  }

  // return the max stack depth size encountered
  int32_t max_depth() const {
    return max_depth_;
  }

protected:
  const int32_t & head() const {
    return var_head_.back();
  }

  int32_t & head() {
    return var_head_.back();
  }

  std::array<identifier_t, 64> vars_;
  std::vector<int32_t> var_head_;
  int32_t max_depth_;

  std::array<identifier_t, 64> args_;
  int32_t arg_head_;
};

struct function_t {
  ccml_syscall_t sys_;
  std::string name_;
  int32_t pos_;
  std::vector<int32_t*> ret_fixup_;
  uint32_t num_args_;

  function_t(const std::string &name, ccml_syscall_t sys, int32_t num_args)
      : sys_(sys), name_(name), pos_(-1), num_args_(num_args) {}

  function_t(const std::string &name, int32_t pos, int32_t num_args)
      : sys_(nullptr), name_(name), pos_(pos), num_args_(num_args) {}

  void add_return_fixup(int32_t *r) {
    ret_fixup_.push_back(r);
  }

  void fix_return_operands(const int32_t count) {
    for (int32_t *r : ret_fixup_) {
      *r = count;
    }
  }
};

struct global_t {
  const token_t *token_;
  int32_t value_;
};

struct parser_t {

  parser_t(ccml_t &c)
    : ccml_(c) {
  }

  // parse all tokens stored in the lexer
  bool parse();

  // reset any stored state
  void reset();

  // add a syscall function
  void add_function(const std::string &name, ccml_syscall_t func, uint32_t num_args);

  // find a function by name
  // if `can_fail == false` it will report an error
  const function_t *find_function(const token_t *name, bool can_fail=false);
  const function_t *find_function(const std::string &name);

  // return a list of globals
  const std::vector<global_t> globals() const {
    return global_;
  }

protected:
  ccml_t &ccml_;

  // list of parsed function
  std::vector<function_t> funcs_;
  // operator stack for expression parsing
  std::vector<token_e> op_stack_;
  // list of parsed globals
  std::vector<global_t> global_;
  // scope manager
  scope_list_t scope_;

  // parse specific language constructs
  void parse_array_get(const token_t &name);
  void parse_array_set(const token_t &name);
  void parse_function();
  void parse_stmt();
  void parse_return();
  void parse_while();
  void parse_if();
  void parse_call(const token_t &name);
  void parse_assign(const token_t &name);
  void parse_decl_array(const token_t &name);
  void parse_decl();
  void parse_expr();
  void parse_expr_ex(uint32_t tide);
  void parse_lhs();
  void parse_global();

  // return true if next token is an operator
  bool is_operator() const;

  // get the precedence of this operator
  int32_t op_type(token_e type) const;

  // load/save a given identifier (local/global)
  void ident_load(const token_t &name);
  void ident_save(const token_t &name);

  // push an operator on the stack
  void op_push(token_e op, uint32_t tide);

  // pop all operators off the stack
  void op_pop_all(uint32_t tide);
};
