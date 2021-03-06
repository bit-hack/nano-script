#pragma once
#include <array>
#include <vector>

#include "nano.h"
#include "token.h"

namespace nano {

enum ast_type_t {
  ast_program_e,
  ast_exp_ident_e,

  ast_exp_lit_float_e,
  ast_exp_lit_var_e,
  ast_exp_lit_str_e,
  ast_exp_none_e,

  ast_block_e,

  ast_exp_array_e,
  ast_exp_call_e,
  ast_exp_bin_op_e,
  ast_exp_unary_op_e,
  ast_exp_array_init_e,
  ast_exp_member_e,

  ast_stmt_if_e,
  ast_stmt_while_e,
  ast_stmt_for_e,
  ast_stmt_return_e,
  ast_stmt_assign_var_e,
  ast_stmt_assign_array_e,
  ast_stmt_call_e,
  ast_stmt_assign_member_e,

  ast_decl_func_e,
  ast_decl_var_e,
  ast_decl_str_e,
};

struct ast_node_t {

  ast_node_t(ast_type_t t)
    : type(t) {
  }

  virtual ~ast_node_t() {}

  template <typename type_t>
  type_t *cast() {
    return is_a<type_t>() ? static_cast<type_t *>(this) : nullptr;
  }

  template <typename type_t>
  const type_t *cast() const {
    return is_a<type_t>() ? static_cast<const type_t *>(this) : nullptr;
  }

  template <typename type_t>
  bool is_a() const {
    if (this == nullptr) {
      return false;
    }
    return type == type_t::TYPE;
  }

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
    (void)which;
    (void)with;
  }

  // node type
  const ast_type_t type;
};

struct ast_program_t : public ast_node_t {
  static const ast_type_t TYPE = ast_program_e;

  ast_program_t()
    : ast_node_t(TYPE)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    for (size_t i = 0; i < children.size(); ++i) {
      if (children[i] == which) {
        children[i] = with;
      }
    }
  }

  std::vector<ast_node_t *> children;
};

struct ast_exp_ident_t: public ast_node_t {
  static const ast_type_t TYPE = ast_exp_ident_e;

  ast_exp_ident_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , decl(nullptr)
  {}

  const token_t *name;
  // where this was declared
  ast_node_t *decl;
};

struct ast_exp_lit_str_t: public ast_node_t {
  static const ast_type_t TYPE = ast_exp_lit_str_e;

  ast_exp_lit_str_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , value(token->str_)
  {}

  ast_exp_lit_str_t(const std::string &value)
    : ast_node_t(TYPE)
    , token(nullptr)
    , value(value)
  {}

  const token_t *token;
  std::string value;
};

struct ast_exp_lit_float_t: public ast_node_t {
  static const ast_type_t TYPE = ast_exp_lit_float_e;

  ast_exp_lit_float_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , val(token->valf_)
  {
  }

  ast_exp_lit_float_t(const float &value)
    : ast_node_t(TYPE)
    , token(nullptr)
    , val(value) {
  }

  const token_t *token;
  float val;
};

struct ast_exp_lit_var_t: public ast_node_t {
  static const ast_type_t TYPE = ast_exp_lit_var_e;

  ast_exp_lit_var_t(const token_t *t)
    : ast_node_t(TYPE)
    , token(t)
    , val(t->val_)
  {
  }

  ast_exp_lit_var_t(const int32_t &value)
    : ast_node_t(TYPE)
    , token(nullptr)
    , val(value)
  {
  }

  const token_t *token;
  int32_t val;
};

struct ast_exp_none_t: public ast_node_t {
  static const ast_type_t TYPE = ast_exp_none_e;

  ast_exp_none_t()
    : ast_node_t(TYPE)
    , token(nullptr)
  {}

  ast_exp_none_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
  {}

  const token_t *token;
};

struct ast_exp_member_t: public ast_node_t {
  static const ast_type_t TYPE = ast_exp_member_e;

  ast_exp_member_t(const token_t *name, const token_t *member)
    : ast_node_t(TYPE)
    , name(name)
    , member(member)
    , decl(nullptr)
  {}

  const token_t *name;
  const token_t *member;
  // where this was declared
  ast_node_t *decl;
};

struct ast_exp_array_init_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_array_init_e;

  ast_exp_array_init_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    for (int i = 0; i < expr.size(); ++i) {
      if (expr[i] == which) {
        expr[i] = with;
      }
    }
  }

  const token_t *name;
  // index expression
  std::vector<ast_node_t *> expr;
};

// XXX: should be called ast_exp_deref_t
struct ast_exp_deref_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_array_e;

  ast_exp_deref_t(ast_node_t *lhs, const token_t *t)
    : ast_node_t(TYPE)
    , token(t)
    , lhs(lhs)
    , index(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    index = (which == index) ? with : index;
    lhs = (which == lhs) ? with : lhs;
  }

  const token_t *token;
  // the object we are dereferencing
  ast_node_t *lhs;
  // index expression
  ast_node_t *index;
};

struct ast_stmt_call_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_call_e;

  ast_stmt_call_t(ast_exp_call_t *expr)
    : ast_node_t(TYPE)
    , expr(expr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    if ((const void*)expr == (const void*)which) {
      expr = with->cast<ast_exp_call_t>();
      assert(expr);
    }
  }

  // an ast_exp_call_t
  ast_exp_call_t *expr;
};

struct ast_exp_call_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_call_e;

  ast_exp_call_t(const token_t *tok)
    : token(tok)
    , callee(nullptr)
    , ast_node_t(TYPE)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i] == which) {
        args[i] = with;
      }
    }
  }

  const token_t *token;
  ast_node_t *callee;

  std::vector<ast_node_t *> args;

  // local frame variables
  std::vector<ast_decl_var_t *> locals;
};

struct ast_exp_bin_op_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_bin_op_e;

  ast_exp_bin_op_t(const token_t *token)
    : ast_node_t(TYPE)
    , op(token->type_)
    , token(token)
    , left(nullptr)
    , right(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    left  = (left  == which) ? with : left;
    right = (right == which) ? with : right;
  }

  token_e op;
  const token_t *token;
  ast_node_t *left, *right;
};

struct ast_exp_unary_op_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_unary_op_e;

  ast_exp_unary_op_t(const token_t *op)
    : ast_node_t(TYPE)
    , op(op)
    , child(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    child = (child  == which) ? with : child;
  }

  const token_t *op;
  ast_node_t *child;
};

struct ast_stmt_if_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_if_e;

  ast_stmt_if_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , expr(nullptr)
    , then_block(nullptr)
    , else_block(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    expr = (expr == which) ? with : expr;
    if ((const void *)then_block == (const void *)which) {
      then_block = with->cast<ast_block_t>();
      assert(then_block);
    }
    if ((const void *)else_block == (const void *)which) {
      else_block = with->cast<ast_block_t>();
      assert(else_block);
    }
  }

  const token_t *token;
  ast_node_t *expr;
  ast_block_t *then_block;
  ast_block_t *else_block;
};

struct ast_block_t: public ast_node_t {
  static const ast_type_t TYPE = ast_block_e;

  ast_block_t()
    : ast_node_t(TYPE)
  {}

  void add(ast_node_t *n) {
    assert(n);
    nodes.push_back(n);
  }

  bool empty() const {
    return nodes.empty();
  }

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    for (size_t i = 0; i < nodes.size(); ++i) {
      if (nodes[i] == which) {
        nodes[i] = with;
      }
    }
  }

  std::vector<ast_node_t*> nodes;
};

struct ast_stmt_while_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_while_e;

  ast_stmt_while_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , expr(nullptr)
    , body(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    expr = (expr == which) ? with : expr;
    if (which == body) {
      body = with->cast<ast_block_t>();
      assert(body);
    }
  }

  const token_t *token;
  ast_node_t *expr;
  ast_block_t *body;
};

struct ast_stmt_for_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_for_e;

  ast_stmt_for_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , name(nullptr)
    , decl(nullptr)
    , start(nullptr)
    , end(nullptr)
    , body(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    start = (start == which) ? with : start;
    end   = (end   == which) ? with : end;
    if (which == body) {
      body = with->cast<ast_block_t>();
      assert(body);
    }
  }

  // the 'for' token
  const token_t *token;

  // loop variable name
  const token_t *name;

  // the loop variable
  ast_decl_var_t *decl;

  // start and end expressions
  ast_node_t *start;
  ast_node_t *end;

  // loop body
  ast_block_t *body;
};

struct ast_stmt_return_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_return_e;

  ast_stmt_return_t(const token_t *token)
    : ast_node_t(TYPE)
    , token(token)
    , expr(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    expr = (expr == which) ? with : expr;
  }

  const token_t *token;
  ast_node_t *expr;
};

struct ast_stmt_assign_var_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_assign_var_e;

  ast_stmt_assign_var_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , expr(nullptr)
    , decl(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    expr = (expr == which) ? with : expr;
  }

  const token_t *name;
  ast_node_t *expr;
  // where was 'name' declared
  ast_decl_var_t *decl;
};

struct ast_stmt_assign_array_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_assign_array_e;

  ast_stmt_assign_array_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , index(nullptr)
    , expr(nullptr)
    , decl(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    index = (index == which) ? with : index;
    expr = (expr == which) ? with : expr;
  }

  const token_t *name;
  ast_node_t *index, *expr;
  // where was 'name' declared
  ast_decl_var_t *decl;
};

struct ast_stmt_assign_member_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_assign_member_e;

  ast_stmt_assign_member_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , member(nullptr)
    , expr(nullptr)
    , decl(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    expr = (expr == which) ? with : expr;
  }

  const token_t *name;
  const token_t *member;
  ast_node_t *expr;
  // where was 'name' declared
  ast_decl_var_t *decl;
};

struct ast_decl_func_t : public ast_node_t {
  static const ast_type_t TYPE = ast_decl_func_e;

  ast_decl_func_t(const token_t *n)
    : ast_node_t(TYPE)
    , token(n)
    , is_syscall(false)
    , is_varargs(false)
    , name(n->str_)
    , body(nullptr)
    , stack_size(0)
    , end(nullptr)
  {}

  ast_decl_func_t(const std::string &n)
    : ast_node_t(TYPE)
    , token(nullptr)
    , is_syscall(false)
    , is_varargs(false)
    , name(n)
    , body(nullptr)
    , stack_size(0)
    , end(nullptr)
  {}

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    auto *w = with->cast<ast_decl_var_t>();
    assert(w);
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i] == w) {
        args[i] = w;
      }
    }
    if (body == which) {
      body = with->cast<ast_block_t>();
      assert(body);
    }
  }

  const token_t *token;
  const token_t *end;

  bool is_syscall;
  bool is_varargs;

  const std::string name;
  std::vector<ast_decl_var_t *> args;
  ast_block_t *body;

  // populated during codegen
  std::vector<ast_decl_var_t*> locals;

  // number of locals
  int32_t stack_size;
};

struct ast_decl_var_t : public ast_node_t {
  static const ast_type_t TYPE = ast_decl_var_e;

  enum scope_t {
    e_local,
    e_global,
    e_arg,
  };

  ast_decl_var_t(const token_t *name, scope_t kind)
    : ast_node_t(TYPE)
    , scope(kind)
    , name(name)
    , expr(nullptr)
    , is_const(false)
    , offset(0)
  {}

  bool is_local() const {
    return scope == e_local;
  }

  bool is_arg() const {
    return scope == e_arg;
  }

  bool is_global() const {
    return scope == e_global;
  }

  void replace_child(const ast_node_t *which, ast_node_t *with) override {
    expr = (expr == which) ? with : expr;
  }

  scope_t scope;

  const token_t *name;

  // if var this could be valid
  ast_node_t *expr;

  bool is_const;

  // global/frame offset for this variable
  uint32_t offset;
};

struct ast_t {

  ast_t(nano_t &nano)
    : nano_(nano)
  {}

  ~ast_t();

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

  // garbage collect
  void gc();

  void dump(FILE *fd);

protected:
  nano_t &nano_;
  std::vector<ast_node_t*> allocs_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct ast_visitor_t {

  virtual void visit(ast_program_t* n) {
    for (auto *c : n->children)
      dispatch(c);
  }

  virtual void visit(ast_exp_ident_t* n) { (void)n; }
  virtual void visit(ast_exp_member_t* n) { (void)n; }
  virtual void visit(ast_exp_lit_var_t* n) { (void)n; }
  virtual void visit(ast_exp_lit_str_t* n) { (void)n; }
  virtual void visit(ast_exp_lit_float_t *n) { (void)n; }
  virtual void visit(ast_exp_none_t* n) { (void)n; }

  virtual void visit(ast_exp_array_init_t* n) {
    for (auto *c : n->expr)
      dispatch(c);
  }

  virtual void visit(ast_exp_deref_t* n) {
    dispatch(n->lhs);
    dispatch(n->index);
  }

  virtual void visit(ast_stmt_call_t* n) {
    dispatch(n->expr);
  }

  virtual void visit(ast_exp_call_t* n) {
    for (auto *c : n->args)
      dispatch(c);
    dispatch(n->callee);
  }

  virtual void visit(ast_exp_bin_op_t* n) {
    dispatch(n->left);
    dispatch(n->right);
  }

  virtual void visit(ast_exp_unary_op_t* n) {
    dispatch(n->child);
  }

  virtual void visit(ast_stmt_if_t* n) {
    dispatch(n->expr);
    if (n->then_block)
      dispatch(n->then_block);
    if (n->else_block)
      dispatch(n->else_block);
  }

  virtual void visit(ast_stmt_while_t* n) {
    dispatch(n->expr);
    dispatch(n->body);
  }

  virtual void visit(ast_stmt_for_t* n) {
    dispatch(n->start);
    dispatch(n->end);
    dispatch(n->body);
  }

  virtual void visit(ast_stmt_return_t* n) {
    dispatch(n->expr);
  }

  virtual void visit(ast_stmt_assign_var_t* n) {
    dispatch(n->expr);
  }

  virtual void visit(ast_stmt_assign_array_t* n) {
    dispatch(n->index);
    if (n->expr) {
      dispatch(n->expr);
    }
  }

  virtual void visit(ast_stmt_assign_member_t* n) {
    dispatch(n->expr);
  }

  virtual void visit(ast_block_t *n) {
    for (auto *a : n->nodes)
      dispatch(a);
  }

  virtual void visit(ast_decl_func_t* n) {
    for (auto *a : n->args)
      dispatch(a);
    dispatch(n->body);
  }

  virtual void visit(ast_decl_var_t* n) {
    dispatch(n->expr);
  }

protected:
  std::vector<ast_node_t*> stack;
  void dispatch(ast_node_t *node);
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct ast_printer_t : ast_visitor_t {

  ast_printer_t(FILE *fd)
    : fd_(fd)
  {}

  ~ast_printer_t() {
    fflush(fd_);
  }

  void visit(ast_program_t* n) override {
    indent_();
    fprintf(fd_, "ast_program_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_member_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_member_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_ident_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_ident_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_lit_float_t *n) override {
    indent_();
    fprintf(fd_, "ast_exp_lit_var_t {value: %f}\n", n->val);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_lit_var_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_lit_var_t {value: %d}\n", n->val);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_lit_str_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_lit_str_t {value: %s}\n", n->value.c_str());
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_none_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_none_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_deref_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_deref_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_call_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_call_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_array_init_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_array_init_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_bin_op_t* n) override {
    indent_();
    const char *op = "?";
    switch (n->op) {
    case TOK_ADD: op = "+";   break;
    case TOK_SUB: op = "-";   break;
    case TOK_MUL: op = "*";   break;
    case TOK_DIV: op = "/";   break;
    case TOK_MOD: op = "%";   break;
    case TOK_LT:  op = "<";   break;
    case TOK_LEQ: op = "<=";  break;
    case TOK_GT:  op = ">";   break;
    case TOK_GEQ: op = ">=";  break;
    case TOK_EQ:  op = "==";  break;
    case TOK_AND: op = "and"; break;
    case TOK_OR:  op = "or";  break;
    default:
      break;
    }
    fprintf(fd_, "ast_exp_bin_op_t {op: %s}\n", op);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_unary_op_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_unary_op_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_block_t* n) override {
    indent_();
    fprintf(fd_, "ast_block_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_if_t* n) override {
    indent_();
    fprintf(fd_, "ast_stmt_if_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_while_t* n) override {
    indent_();
    fprintf(fd_, "ast_stmt_while_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_for_t* n) override {
    indent_();
    fprintf(fd_, "ast_stmt_for_t {name=%s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_assign_member_t* n) override {
    indent_();
    fprintf(fd_, "ast_stmt_assign_member_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_return_t* n) override {
    indent_();
    fprintf(fd_, "ast_stmt_return_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_assign_var_t* n) override {
    indent_();
    fprintf(fd_, "ast_stmt_assign_var_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_assign_array_t* n) override {
    indent_();
    fprintf(fd_, "ast_stmt_assign_array_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_call_t* n) override {
    indent_();
    fprintf(fd_, "ast_stmt_call_t\n");
    ast_visitor_t::visit(n);
  }

  void visit(ast_decl_func_t* n) override {
    indent_();
    fprintf(fd_, "ast_decl_func_t {name: %s}\n", n->name.c_str());
    ast_visitor_t::visit(n);
  }

  void visit(ast_decl_var_t* n) override {
    indent_();
    fprintf(fd_, "ast_decl_var_t {name: %s}\n", n->name->string());
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

} // namespace nano
