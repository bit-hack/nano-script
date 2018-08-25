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

  scope_list_t() {
    head_.emplace_back(0);
  }

  void enter() {
    head_.emplace_back(head_.size());
  }

  void leave() {
    head_.pop_back();
  }

  void add(const token_t *tok) {
    assert(head() < int32_t(scope_.size()));
    int32_t offset = 0;
    scope_[head()++] = identifier_t {offset, tok};
  }

  const identifier_t *find(const std::string &name) const {
    for (int32_t i = head(); i >= 0; --i) {
      if (scope_[i].token->str_ == name) {
        return &scope_[i];
      }
    }
    return nullptr;
  }

protected:
  const int32_t & head() const {
    return head_.back();
  }

  int32_t & head() {
    return head_.back();
  }

  std::array<identifier_t, 64> scope_;
  std::vector<int32_t> head_;
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
