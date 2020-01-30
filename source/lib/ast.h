#pragma once
#include <array>
#include <vector>

#include "ccml.h"
#include "token.h"

namespace ccml {

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
  ast_stmt_if_e,
  ast_stmt_while_e,
  ast_stmt_for_e,
  ast_stmt_return_e,
  ast_stmt_assign_var_e,
  ast_stmt_assign_array_e,
  ast_stmt_call_e,
  ast_decl_func_e,
  ast_decl_var_e,
  ast_decl_str_e,
  ast_array_init_e,
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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
  ast_decl_var_t *decl;
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

struct ast_exp_array_t : public ast_node_t {
  static const ast_type_t TYPE = ast_exp_array_e;

  ast_exp_array_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , index(nullptr)
    , decl(nullptr)
  {}

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
    index = (which == index) ? with : index;
  }

  const token_t *name;
  // index expression
  ast_node_t *index;
  // where this was declared
  ast_decl_var_t *decl;
};

struct ast_stmt_call_t : public ast_node_t {
  static const ast_type_t TYPE = ast_stmt_call_e;

  ast_stmt_call_t(ast_exp_call_t *expr)
    : ast_node_t(TYPE)
    , expr(expr)
  {}

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  ast_exp_call_t(const token_t *name)
    : ast_node_t(TYPE)
    , name(name)
    , is_syscall(false)
    , decl(nullptr)
  {}

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i] == which) {
        args[i] = with;
      }
    }
  }

  const token_t *name;
  bool is_syscall;
  ast_decl_func_t *decl;
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
    index = (index == which) ? with : index;
    expr = (expr == which) ? with : expr;
  }

  const token_t *name;
  ast_node_t *index, *expr;
  // where was 'name' declared
  ast_decl_var_t *decl;
};

struct ast_array_init_t: public ast_node_t {
  static const ast_type_t TYPE = ast_array_init_e;

  ast_array_init_t()
    : ast_node_t(TYPE)
  {}

  std::vector<const token_t *> item;
};

struct ast_decl_func_t : public ast_node_t {
  static const ast_type_t TYPE = ast_decl_func_e;

  ast_decl_func_t(const token_t *n)
    : ast_node_t(TYPE)
    , token(n)
    , name(n->str_)
    , body(nullptr)
    , stack_size(0)
  {}

  ast_decl_func_t(const std::string &n)
    : ast_node_t(TYPE)
    , token(nullptr)
    , name(n)
    , body(nullptr)
  {}

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
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
  const std::string name;
  std::vector<ast_decl_var_t *> args;
  ast_block_t *body;

  // populated during codegen
  std::vector<ast_decl_var_t*> locals;

  // number of locals
  uint32_t stack_size;
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
    , size(nullptr)
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

  bool is_array() const {
    return size != nullptr;
  }

  int32_t count() const {
    if (!size) {
      return 1;
    }
    else {
      const auto *v = size->cast<ast_exp_lit_var_t>();
      return v->val;
    }
  }

  virtual void replace_child(const ast_node_t *which, ast_node_t *with) {
    expr = (expr == which) ? with : expr;
    size = (size == which) ? with : size;
  }

  scope_t scope;

  const token_t *name;

  // if var this could be valid
  ast_node_t *expr;
  // size if array
  ast_node_t *size;

  bool is_const;

  // global/frame offset for this variable
  uint32_t offset;
};

struct ast_t {

  ast_t(ccml_t &ccml)
    : ccml_(ccml)
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
  ccml_t &ccml_;
  std::vector<ast_node_t*> allocs_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct ast_visitor_t {

  virtual void visit(ast_program_t* n) {
    for (auto *c : n->children)
      dispatch(c);
  }

  virtual void visit(ast_exp_ident_t* n) { (void)n; }
  virtual void visit(ast_exp_lit_var_t* n) { (void)n; }
  virtual void visit(ast_exp_lit_str_t* n) { (void)n; }
  virtual void visit(ast_exp_lit_float_t *n) { (void)n; }
  virtual void visit(ast_exp_none_t* n) { (void)n; }
  virtual void visit(ast_array_init_t* n) { (void)n; }

  virtual void visit(ast_exp_array_t* n) {
    dispatch(n->index);
  }

  virtual void visit(ast_stmt_call_t* n) {
    dispatch(n->expr);
  }

  virtual void visit(ast_exp_call_t* n) {
    for (auto *c : n->args)
      dispatch(c);
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
    if (n->size) {
      dispatch(n->size);
    }
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

  void visit(ast_exp_array_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_array_t {name: %s}\n", n->name->string());
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_call_t* n) override {
    indent_();
    fprintf(fd_, "ast_exp_call_t {name: %s}\n", n->name->string());
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

  void visit(ast_array_init_t* n) override {
    indent_();
    fprintf(fd_, "ast_array_init_t {size: %d}\n", int(n->item.size()));
    ast_visitor_t::visit(n);
  }

  void visit(ast_decl_var_t* n) override {
    indent_();
    fprintf(fd_, "ast_decl_var_t {name: %s, size:%d}\n", n->name->string(), int(n->count()));
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
