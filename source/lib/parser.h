#pragma once
#include "ccml.h"
#include "token.h"

struct parser_t {

  struct function_t {
    ccml_syscall_t sys_;
    std::string name_;
    int32_t pos_;
    int32_t frame_;
    std::vector<std::string> ident_;

    function_t() : sys_(nullptr), name_(), pos_(), frame_() {}

    int32_t frame_size() const { return ident_.size(); }

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
    token_t *token_;
    int32_t value_;
  };

  parser_t(ccml_t &c) : ccml_(c) {}

  // parse all tokens stored in the lexer
  bool parse();

  // reset any stored state
  void reset();

  // add a syscall function
  void add_function(const std::string &name, ccml_syscall_t func);

  // find a function by name
  const function_t *find_function(const std::string &name);

  // return a list of globals
  const std::vector<global_t> globals() const {
    return global_;
  }

protected:
  ccml_t &ccml_;

  std::vector<function_t> funcs_;
  std::vector<token_e> op_stack_;
  std::vector<global_t> global_;

  void parse_function();
  void parse_stmt();
  void parse_while();
  void parse_if();
  void parse_call(token_t *name);
  void parse_assign(token_t *name);
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
