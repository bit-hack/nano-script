#pragma once
#include <array>
#include <vector>

#include "ccml.h"
#include "token.h"

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
  ast_stmt_call_e,
  ast_decl_func_e,
  ast_decl_var_e,
};

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
    return type == type_t::TYPE;
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


struct ast_stmt_call_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_call_e;

  ast_stmt_call_t(ast_node_t *expr)
    : ast_node_t(TYPE)
    , expr(nullptr)
  {}

  ast_node_t *expr;
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
    , name(name)
  {}

  const token_t *name;
  std::vector<ast_node_t *> args;
  std::vector<ast_node_t *> body;
};

struct ast_decl_var_t : public ast_node_t {
  static const ast_type_t TYPE = ast_decl_var_e;

  enum kind_t {
    e_local,
    e_global,
    e_arg
  };

  ast_decl_var_t(const token_t *name, kind_t kind)
    : ast_node_t(TYPE)
    , name(name)
    , expr(nullptr)
    , size(nullptr)
    , is_arg_(false)
    , kind(kind)
  {}

  bool is_local() const {
    return kind == e_local;
  }

  bool is_arg() const {
    return kind == e_arg;
  }

  bool is_global() const {
    return kind == e_global;
  }

  bool is_array() const {
    return size != nullptr;
  }

  int32_t count() const {
    return size ? size->val_ : 1;
  }

  kind_t kind;

  const token_t *name;
  // if var this could be valid
  ast_node_t *expr;
  // if array this will be valid
  const token_t *size;
  bool is_arg_;
};

struct ast_t {

  ast_t(ccml_t &ccml)
    : ccml_(ccml)
  {}

  template <typename T, class... Types>
  T *alloc(Types &&... args) {
    static_assert(std::is_base_of<ast_node_t, T>::value,
      "type must derive from ast_node_t");
    T *obj = new T(std::forward<Types>(args)...);
    allocs_.push_back(obj);
    return obj;
  }

  ast_program_t program;

  void reset();

protected:
  ccml_t &ccml_;
  std::vector<ast_node_t*> allocs_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct ast_visitor_t {

  virtual void visit(ast_program_t* n) {
    stack.push_back(n);
    for (auto *c : n->children)
      dispatch(c);
    stack.pop_back();
  }

  virtual void visit(ast_exp_ident_t* n) {}
  virtual void visit(ast_exp_const_t* n) {}
  virtual void visit(ast_exp_array_t* n) {}

  virtual void visit(ast_stmt_call_t* n) {
    stack.push_back(n);
    dispatch(n->expr);
    stack.pop_back();
  }

  virtual void visit(ast_exp_call_t* n) {
    stack.push_back(n);
    for (auto *c : n->args)
      dispatch(c);
    stack.pop_back();
  }

  virtual void visit(ast_exp_bin_op_t* n) {
    stack.push_back(n);
    dispatch(n->left);
    dispatch(n->right);
    stack.pop_back();
  }

  virtual void visit(ast_exp_unary_op_t* n) {
    stack.push_back(n);
    dispatch(n->child);
    stack.pop_back();
  }

  virtual void visit(ast_stmt_if_t* n) {
    stack.push_back(n);
    dispatch(n->expr);
    for (auto *c : n->then_block)
      dispatch(c);
    for (auto *c : n->else_block)
      dispatch(c);
    stack.pop_back();
  }

  virtual void visit(ast_stmt_while_t* n) {
    stack.push_back(n);
    dispatch(n->expr);
    for (auto *c : n->body)
      dispatch(c);
    stack.pop_back();
  }

  virtual void visit(ast_stmt_return_t* n) {
    stack.push_back(n);
    dispatch(n->expr);
    stack.pop_back();
  }

  virtual void visit(ast_stmt_assign_var_t* n) {
    stack.push_back(n);
    dispatch(n->expr);
    stack.pop_back();
  }

  virtual void visit(ast_stmt_assign_array_t* n) {}

  virtual void visit(ast_decl_func_t* n) {
    stack.push_back(n);
    for (auto *c : n->body)
      dispatch(c);
    stack.pop_back();
  }

  virtual void visit(ast_decl_var_t* n) {
    stack.push_back(n);
    dispatch(n->expr);
    stack.pop_back();
  }

protected:
  std::vector<ast_node_t*> stack;
  void dispatch(ast_node_t *node);
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct ast_printer_t : ast_visitor_t {
  
  ast_printer_t()
    : fd_(stderr)
  {}

  ~ast_printer_t() {
    fflush(fd_);
  }

  virtual void visit(ast_program_t* n) {
    indent_();
    printf("ast_program_t\n");
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_exp_ident_t* n) {
    indent_();
    printf("ast_exp_ident_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_exp_const_t* n) {
    indent_();
    printf("ast_exp_const_t {value: %d}\n", n->value->val_);
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_exp_array_t* n) {
    indent_();
    printf("ast_exp_array_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_exp_call_t* n) {
    indent_();
    printf("ast_exp_call_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_exp_bin_op_t* n) {
    indent_();
    printf("ast_exp_bin_op_t\n");
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_exp_unary_op_t* n) {
    indent_();
    printf("ast_exp_unary_op_t\n");
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_stmt_if_t* n) {
    indent_();
    printf("ast_stmt_if_t\n");
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_stmt_while_t* n) {
    indent_();
    printf("ast_stmt_while_t\n");
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_stmt_return_t* n) {
    indent_();
    printf("ast_stmt_return_t\n");
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_stmt_assign_var_t* n) {
    indent_();
    printf("ast_stmt_assign_var_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_stmt_assign_array_t* n) {
    indent_();
    printf("ast_stmt_assign_array_t\n");
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_stmt_call_t* n) {
    indent_();
    printf("ast_stmt_call_t\n");
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_decl_func_t* n) {
    indent_();
    printf("ast_decl_func_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  virtual void visit(ast_decl_var_t* n) {
    indent_();
    printf("ast_decl_var_t {name: %s, size:%d}\n", n->name->string(), n->count());
    ast_visitor_t::visit(n);
  }

protected:
  FILE *fd_;

  void indent_() {
    for (size_t i = 0; i < stack.size(); ++i) {
      fputs(".  ", fd_);
    }
  }
};

} // namespace ccml
