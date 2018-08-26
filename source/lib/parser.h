#pragma once
#include <vector>
#include <array>

#include "ccml.h"
#include "token.h"


struct identifier_t {
  // offset from frame pointer
  int32_t offset;
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

  void var_add(const token_t *tok) {
    assert(head() < int32_t(vars_.size()));
    const int32_t offset = head();
    vars_[head()++] = identifier_t {offset, tok};
    // update max depth
    max_depth_ = std::max(max_depth_, head());
  }

  int32_t var_count() const {
    return head();
  }

  void arg_add(const token_t *tok) {
    identifier_t ident{0, tok};
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

struct parser_t {

  struct function_t {
    ccml_syscall_t sys_;
    std::string name_;
    int32_t pos_;
    int32_t frame_;
    std::vector<std::string> ident_;

    function_t()
      : sys_(nullptr)
      , name_()
      , pos_()
      , frame_() {
    }

    int32_t frame_size() const {
      return ident_.size();
    }

    // find an identifier and return its stack index relative to fp_
    bool find(const std::string &str, int32_t &index) {
      for (uint32_t i = 0; i < ident_.size(); i++) {
        if (str == ident_[i]) {
          index = i - frame_;
          return true;
        }
      }
      return false;
    }
  };

  struct global_t {
    const token_t *token_;
    int32_t value_;
  };

  parser_t(ccml_t &c)
    : ccml_(c) {
  }

  // parse all tokens stored in the lexer
  bool parse();

  // reset any stored state
  void reset();

  // add a syscall function
  void add_function(const std::string &name, ccml_syscall_t func);

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
  void parse_function();
  void parse_stmt();
  void parse_return();
  void parse_while();
  void parse_if();
  void parse_call(const token_t *name);
  void parse_assign(const token_t *name);
  void parse_decl();
  void parse_expr();
  void parse_expr_ex(uint32_t tide);
  void parse_lhs();
  void parse_global();

  // return true if next token is an operator
  bool is_operator();

  // get the precedence of this operator
  int32_t op_type(token_e type);

  // load from a given identifier (local/global)
  void load_ident(const token_t &name);

  // push an operator on the stack
  void op_push(token_e op, uint32_t tide);

  // pop all operators off the stack
  void op_popall(uint32_t tide);

  // get the current function being parsed
  function_t &func();
};
