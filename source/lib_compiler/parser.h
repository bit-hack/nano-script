#pragma once
#include <vector>
#include <array>

#include "nano.h"
#include "token.h"


namespace nano {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct parser_t {

  parser_t(nano_t &c);

  // parse all tokens stored in the lexer
  bool parse(struct error_t &error);

  // reset any stored state
  void reset();

protected:
  nano_t &nano_;

  // operator stack for expression parsing
  std::vector<const token_t*> op_stack_;
  // expression stack
  std::vector<ast_node_t*> exp_stack_;

  // parse specific language constructs
  ast_node_t* parse_array_get_(const token_t &name);
  ast_node_t* parse_array_set_(const token_t &name);
  ast_node_t* parse_function_(const token_t &t);
  ast_node_t* parse_stmt_();
  ast_node_t* parse_return_(const token_t &t);
  ast_node_t* parse_while_(const token_t &t);
  ast_node_t* parse_for_(const token_t &t);
  ast_node_t* parse_if_(const token_t &t);
  ast_exp_call_t* parse_call_(const token_t &t);
  ast_node_t* parse_compound_(const token_t &t);
  ast_node_t* parse_assign_(const token_t &name);
  ast_node_t* parse_decl_array_(const token_t &name);
  ast_node_t* parse_decl_var_(const token_t &t);
  ast_node_t* parse_expr_();
  ast_node_t* parse_const_(const token_t &var);
  ast_node_t* parse_global_(const token_t &t);
  bool parse_import_(const token_t &t);

  // expressions parsing helpers
  void parse_expr_ex_(uint32_t tide);
  void parse_lhs_();

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

} // namespace nano
