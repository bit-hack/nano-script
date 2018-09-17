#pragma once
#include <vector>
#include <array>

#include "ccml.h"
#include "token.h"
// #include "codegen.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct function_t {
  ccml_syscall_t sys_;
  std::string name_;
  int32_t pos_;
  uint32_t num_args_;
  uint32_t id_;
  std::vector<int32_t*> ret_fixup_;

  function_t(const std::string &name, ccml_syscall_t sys, int32_t num_args,
             int32_t id)
    : sys_(sys), name_(name), pos_(-1), num_args_(num_args), id_(id) {}

  function_t(const std::string &name, int32_t pos, int32_t num_args, int32_t id)
    : sys_(nullptr), name_(name), pos_(pos), num_args_(num_args), id_(id) {}

  void add_return_fixup(int32_t *r) {
    ret_fixup_.push_back(r);
  }

  void fix_return_operands(const int32_t count) {
    for (int32_t *r : ret_fixup_) {
      *r = count;
    }
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct global_t {
  const token_t *token_;
  int32_t offset_;
  int32_t value_;
  int32_t size_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct parser_t {

  parser_t(ccml_t &c);

  // parse all tokens stored in the lexer
  bool parse(struct error_t &error);

  // reset any stored state
  void reset();

  // add a syscall function
  void add_function(const std::string &name, ccml_syscall_t func, uint32_t num_args);

  // find a function by name
  // if `can_fail == false` it will report an error
  const function_t *find_function(const token_t *name, bool can_fail = false) const;
  const function_t *find_function(const std::string &name) const;
  const function_t *find_function(uint32_t id) const;

  // return a list of functions
  const std::vector<function_t> &functions() const {
    return funcs_;
  }

  // return the number of globals
  int32_t global_size() const {
    int32_t count = 0;
    for (const auto &g : globals()) {
      count += g.size_;
    }
    return count;
  }

  // return a list of globals
  const std::vector<global_t> globals() const {
    return global_;
  }

protected:
  ccml_t &ccml_;

  // list of parsed function
  std::vector<function_t> funcs_;

  // operator stack for expression parsing
  std::vector<const token_t*> op_stack_;
  // expression stack
  std::vector<ast_node_t*> exp_stack_;

    // list of parsed globals
  std::vector<global_t> global_;

  // parse specific language constructs
  ast_node_t* parse_array_get_(const token_t &name);
  ast_node_t* parse_array_set_(const token_t &name);
  ast_node_t* parse_function_(const token_t &t);
  ast_node_t* parse_stmt_();
  ast_node_t* parse_return_(const token_t &t);
  ast_node_t* parse_while_(const token_t &t);
  ast_node_t* parse_if_(const token_t &t);
  ast_node_t* parse_call_(const token_t &name);
  ast_node_t* parse_assign_(const token_t &name);
  ast_node_t* parse_decl_array_(const token_t &name);
  ast_node_t* parse_decl_(const token_t &t);
  ast_node_t* parse_expr_();
  void parse_expr_ex_(uint32_t tide);
  void parse_lhs_();
  ast_node_t* parse_global_();

  // return true if next token is an operator
  bool is_operator_() const;

  // get the precedence of this operator
  int32_t op_type_(token_e type) const;

  // XXX: move into subclass
  // push an operator on the stack
  void op_push_(const token_t* op, uint32_t tide);
  // pop all operators off the stack
  void op_pop_all_(uint32_t tide);
  // reduce an operation
  void op_reduce_();
};

} // namespace ccml
