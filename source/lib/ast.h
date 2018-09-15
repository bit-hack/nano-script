#pragma once
#include <array>
#include <vector>

#include "ccml.h"

namespace ccml {

enum ast_type_t {
  ast_program_e,
  ast_exp_ident_e,
  ast_exp_const_e,
  ast_exp_array_e,
  ast_exp_call_e,
  ast_exp_bin_op_e,
  ast_exp_unary_op_e,
  ast_stmt_if_e,
  ast_stmt_while_e,
  ast_stmt_return_e,
  ast_stmt_assign_var_e,
  ast_stmt_assign_array_e,
  ast_decl_func_e,
  ast_decl_var_e,
  ast_decl_array_e,
};

struct ast_node_t;
struct ast_program_t;
struct ast_exp_ident_t;

struct ast_node_t {

  ast_node_t(ast_type_t t)
    : type(t) {
  }

  template <typename type_t>
  type_t *cast() {
    return is_a<type_t>() ? static_cast<type_t *>(this) : nullptr;
  }

  template <typename type_t>
  bool is_a() {
    return type_ == type_t::TYPE;
  }

  const ast_type_t type;
};

struct ast_program_t : public ast_node_t {
  static const ast_type_t TYPE = ast_program_e;

  ast_program_t()
    : ast_node_t(TYPE)
  {}

  std::vector<ast_node_t *> children;
};

struct ast_exp_ident_t: public ast_node_t {
  static const ast_type_t TYPE = ast_exp_ident_e;

  ast_exp_ident_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
  {}

  const token_t *name;
};

struct ast_exp_const_t: public ast_node_t {
  static const ast_type_t TYPE = ast_exp_const_e;

  ast_exp_const_t(const token_t *value)
    : ast_node_t(TYPE)
    , value(value)
  {}

  const token_t *value;
};

struct ast_exp_array_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_array_e;

  ast_exp_array_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , index(nullptr)
  {}

  const token_t *name;
  ast_node_t *index;
};

struct ast_exp_call_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_call_e;

  ast_exp_call_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
  {}

  const token_t *name;
  std::vector<ast_node_t *> args;
};

struct ast_exp_bin_op_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_bin_op_e;

  ast_exp_bin_op_t(const token_t *op)
    : ast_node_t(TYPE)
    , op(op)
    , left(nullptr)
    , right(nullptr)
  {}

  const token_t *op;
  ast_node_t *left, *right;
};

struct ast_exp_unary_op_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_unary_op_e;

  ast_exp_unary_op_t(const token_t *op)
    : ast_node_t(TYPE)
    , op(op)
    , child(nullptr)
  {}

  const token_t *op;
  ast_node_t *child;
};

struct ast_stmt_if_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_if_e;

  ast_stmt_if_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , expr(nullptr)
  {}

  const token_t *token;
  ast_node_t *expr;
  std::vector<ast_node_t*> then_block, else_block;
};

struct ast_stmt_while_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_while_e;

  ast_stmt_while_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , expr(nullptr)
  {}

  const token_t *token;
  ast_node_t *expr;
  std::vector<ast_node_t*> body;
};

struct ast_stmt_return_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_return_e;

  ast_stmt_return_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , expr(nullptr)
  {}

  const token_t *token;
  ast_node_t *expr;
};

struct ast_stmt_assign_var_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_assign_var_e;

  ast_stmt_assign_var_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , expr(nullptr)
  {}

  const token_t *name;
  ast_node_t *expr;
};

struct ast_stmt_assign_array_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_assign_array_e;

  ast_stmt_assign_array_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , index(nullptr)
    , expr(nullptr)
  {}

  const token_t *name;
  ast_node_t *index, *expr;
};

struct ast_decl_func_t : public ast_node_t {
  static const ast_type_t TYPE = ast_decl_func_e;

  ast_decl_func_t(const token_t *name)
    : ast_node_t(TYPE)
  {}

  const token_t *name;
  std::vector<const token_t *> args;
  std::vector<ast_node_t *> body;
};

struct ast_decl_var_t : public ast_node_t {
  static const ast_type_t TYPE = ast_decl_var_e;

  ast_decl_var_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , expr(nullptr)
  {}

  const token_t *name;
  ast_node_t *expr;
};

struct ast_decl_array_t : public ast_node_t {
  static const ast_type_t TYPE = ast_decl_array_e;

  ast_decl_array_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , size(size)
  {}

  const token_t *name;
  const token_t *size;
};

} // namespace ccml
