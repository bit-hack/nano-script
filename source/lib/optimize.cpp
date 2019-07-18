#include <set>

#include "sema.h"
#include "ast.h"
#include "errors.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// redundant decl elimination
//
struct op_decl_elim_t: public ast_visitor_t {

  op_decl_elim_t(ccml_t &ccml)
    : errs_(ccml.errors())
    , ast_(ccml.ast())
    , removing_(false)
    , decl_(nullptr)
  {}

  std::set<const ast_decl_var_t*> uses_;
  bool removing_;
  ast_decl_var_t *decl_;

  void visit(ast_decl_func_t *n) override {
    uses_.clear();
    // collect all uses
    removing_ = false;
    ast_visitor_t::visit(n);
    // remove an unused decls
    removing_ = true;
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_call_t *n) override {
    // if a decl expr calls a function we cant remove it
    if (decl_ && !removing_) {
      uses_.insert(decl_);
    }
  }

  void visit(ast_decl_var_t *n) override {
    decl_ = removing_ ? nullptr : n;
    ast_visitor_t::visit(n);
    decl_ = nullptr;
  }

  void visit(ast_stmt_if_t *n) override {
    if (removing_) {
      for (auto itt = n->then_block.begin(); itt != n->then_block.end();) {
        ast_decl_var_t *a = (*itt)->cast<ast_decl_var_t>();
        if (uses_.count(a) == 0) {
          itt = n->then_block.erase(itt);
        } else {
          ++itt;
        }
      }
      for (auto itt = n->else_block.begin(); itt != n->else_block.end();) {
        ast_decl_var_t *a = (*itt)->cast<ast_decl_var_t>();
        if (uses_.count(a) == 0) {
          itt = n->then_block.erase(itt);
        } else {
          ++itt;
        }
      }
    }
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_while_t *n) override {
    if (removing_) {
      for (auto itt = n->body.begin(); itt != n->body.end();) {
        ast_decl_var_t *a = (*itt)->cast<ast_decl_var_t>();
        if (uses_.count(a) == 0) {
          itt = n->body.erase(itt);
        } else {
          ++itt;
        }
      }
    }
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_ident_t *n) override {
    if (!removing_) {
      // find the decl
      const ast_decl_var_t *decl = n->decl->cast<const ast_decl_var_t>();
      if (decl) {
        uses_.insert(decl);
      }
    }
  }

  error_manager_t &errs_;
  ast_t &ast_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// redundant store elimination
//
struct op_store_elim_t: public ast_visitor_t {

  op_store_elim_t(ccml_t &ccml)
    : errs_(ccml.errors())
    , ast_(ccml.ast()) {}

  void visit(ast_decl_func_t *n) override {
    // has a body
  }

  void visit(ast_stmt_while_t *n) override {
    // has a body
  }

  void visit(ast_stmt_if_t *n) override {
    // has a body
  }

  error_manager_t &errs_;
  ast_t &ast_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// constant propagation
//
struct opt_const_expr_t: public ast_visitor_t {

  opt_const_expr_t(ccml_t &ccml)
    : errs_(ccml.errors())
    , ast_(ccml.ast()) {}

  void visit(ast_exp_bin_op_t *o) override {
    dispatch(o->left);
    dispatch(o->right);
    // evaluate this node
    int32_t v = 0;
    eval_(o, v);
  }

  void visit(ast_exp_unary_op_t *o) override {
    dispatch(o->child);
    int32_t v = 0;
    eval_(o, v);
  }

  void visit(ast_stmt_if_t *s) override {
    ast_visitor_t::visit(s);
    int32_t v = 0;
    if (auto *e = s->expr->cast<ast_exp_bin_op_t>()) {
      if (eval_(e, v)) {
        s->expr = ast_.alloc<ast_exp_lit_var_t>(v);
      }
    }
  }

  void visit(ast_stmt_while_t *s) override {
    ast_visitor_t::visit(s);
    int32_t v = 0;
    if (auto *e = s->expr->cast<ast_exp_bin_op_t>()) {
      if (eval_(e, v)) {
        s->expr = ast_.alloc<ast_exp_lit_var_t>(v);
      }
    }
  }

  void visit(ast_stmt_return_t *s) override {
    ast_visitor_t::visit(s);
    int32_t v = 0;
    if (auto *e = s->expr->cast<ast_exp_bin_op_t>()) {
      if (eval_(e, v)) {
        s->expr = ast_.alloc<ast_exp_lit_var_t>(v);
      }
    }
  }

  void visit(ast_stmt_assign_var_t *s) override {
    ast_visitor_t::visit(s);
    int32_t v = 0;
    if (auto *e = s->expr->cast<ast_exp_bin_op_t>()) {
      if (eval_(e, v)) {
        s->expr = ast_.alloc<ast_exp_lit_var_t>(v);
      }
    }
  }

  void visit(ast_program_t *p) override {
    ast_visitor_t::visit(p);
  }

protected:

  std::map<const ast_node_t *, int32_t> val_;

  bool value_(const ast_node_t *n, int32_t &v) const {
    if (const auto *e = n->cast<ast_exp_lit_var_t>()) {
      v = e->value;
      return true;
    }
    auto itt = val_.find(n);
    if (itt != val_.end()) {
      v = itt->second;
      return true;
    }
    return false;
  }

  bool eval_(const ast_exp_unary_op_t *o, int32_t &v) {
    // get operand
    int32_t a;
    if (value_(o->child, a)) {
      // evaluate operator
      switch (o->op->type_) {
      case TOK_SUB: v = -a; break;
      case TOK_NOT: v = !a; break;
      default: assert(!"unknown operator");
      }
      val_[o] = v;
      return true;
    }
    return false;
  }

  bool eval_(const ast_exp_bin_op_t *o, int32_t &v) {
    // get opcode
    const auto op = o->op;
    // get operands
    int32_t a, b;
    const bool vl = value_(o->left, a);
    const bool vb = value_(o->right, b);
#if 0
    // note: disabled because it could skip function calls
    if (op == TOK_AND) {
      if ((vl && a == 0) || (vb && b == 0) {
        val_[o] = 0;
        return true;
      }
    }
    if (op == TOK_OR) {
      if ((vl && a != 0) || (vb && b != 0)) {
        val_[o] = 1;
        return true;
      }
    }
#endif
    if (vl && vb) {
      // check for constant division
      if (b == 0) {
        if (op == TOK_DIV || op == TOK_MOD) {
          errs_.constant_divie_by_zero(*o->token);
        }
      }
      // evaluate operator
      switch (o->op) {
      case TOK_ADD: v = a +  b; break;
      case TOK_SUB: v = a -  b; break;
      case TOK_MUL: v = a *  b; break;
      case TOK_AND: v = a && b; break;
      case TOK_OR:  v = a || b; break;
      case TOK_LEQ: v = a <= b; break;
      case TOK_GEQ: v = a >= b; break;
      case TOK_LT:  v = a <  b; break;
      case TOK_GT:  v = a >  b; break;
      case TOK_EQ:  v = a == b; break;
      case TOK_DIV: v = a /  b; break;
      case TOK_MOD: v = a %  b; break;
      default: assert(!"unknown operator");
      }
      val_[o] = v;
      return true;
    }
    return false;
  }

  error_manager_t &errs_;
  ast_t &ast_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// delete dead code after a return statement
//
struct opt_post_ret_t: public ast_visitor_t {

  opt_post_ret_t(ccml_t &ccml)
    : errs_(ccml.errors()) {}

  void visit(ast_stmt_while_t *w) override {
    simplify_(w->body);
    ast_visitor_t::visit(w);
  }

  void visit(ast_stmt_if_t *d) override {
    simplify_(d->then_block);
    simplify_(d->else_block);
    ast_visitor_t::visit(d);
  }

  void visit(ast_decl_func_t *f) override {
    simplify_(f->body);
    ast_visitor_t::visit(f);
  }

  void visit(ast_program_t *p) override {
    ast_visitor_t::visit(p);
  }

protected:
  error_manager_t &errs_;

  void simplify_(std::vector<ast_node_t*> &body) {
    bool found_return = false;
    for (auto i = body.begin(); i != body.end();) {
      ast_node_t *n = *i;
      if (found_return) {
        i = body.erase(i);
      }
      else {
        found_return = n->is_a<ast_stmt_return_t>();
        ++i;
      }
    }
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// remove unreachable branches
//
struct opt_if_remove_t: public ast_visitor_t {

  opt_if_remove_t(ccml_t &ccml)
    : errs_(ccml.errors()) {}

  void visit(ast_stmt_if_t *s) override {
    ast_visitor_t::visit(s);
    if (auto *c = s->expr->cast<ast_exp_lit_var_t>()) {
      if (c->value != 0) {
        s->else_block.clear();
      }
      else {
        s->then_block.clear();
      }
    }
  }

  void visit(ast_stmt_while_t *s) override {
    ast_visitor_t::visit(s);
    if (auto *c = s->expr->cast<ast_exp_lit_var_t>()) {
      if (c->value == 0) {
        s->body.clear();
      }
    }
  }

  void visit(ast_program_t *p) override {
    ast_visitor_t::visit(p);
  }

protected:
  error_manager_t &errs_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// commutative tree rotation
//
struct opt_com_rotation_t: public ast_visitor_t {

  opt_com_rotation_t(ccml_t &ccml)
    : ccml_(ccml)
  {}

  void visit(ast_exp_lit_var_t *n) override {
    depth_ = 0;    // terminal node
  }

  void visit(ast_exp_lit_str_t *n) override {
    depth_ = 0;    // terminal node
  }

  void visit(ast_exp_call_t *n) override {
    depth_ = 0;    // terminal node
  }

  void visit(ast_exp_bin_op_t *o) override {
    assert(o->left || o->right);
    // depth of both subtrees
    const int32_t dl = depth_;
    dispatch(o->left);
    const int32_t dr = depth_;
    dispatch(o->right);
    // if its a commutative operator
    switch (o->op) {
    case TOK_ADD:
    case TOK_MUL:
    case TOK_AND:
    case TOK_OR:
      if (dr > dl) {
        std::swap(o->left, o->right);
      }
    }
    ++depth_;
  }

  void visit(ast_exp_unary_op_t *o) override {
    dispatch(o->child);
    ++depth_;
  }

  void visit(ast_program_t *p) override {
    ast_visitor_t::visit(p);
  }

  int32_t depth_;
  ccml_t &ccml_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void run_optimize(ccml_t &ccml) {
  opt_post_ret_t    (ccml).visit(&(ccml.ast().program));
  opt_const_expr_t  (ccml).visit(&(ccml.ast().program));
  opt_if_remove_t   (ccml).visit(&(ccml.ast().program));
  opt_com_rotation_t(ccml).visit(&(ccml.ast().program));
}

} // namespace ccml
