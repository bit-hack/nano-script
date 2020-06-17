#include <set>

#include "ast.h"
#include "errors.h"


namespace nano {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// redundant decl elimination
//
struct op_decl_elim_t: public ast_visitor_t {

  op_decl_elim_t(nano_t &nano)
    : errs_(nano.errors())
    , ast_(nano.ast())
    , removing_(false)
    , decl_(nullptr)
  {}

  void visit(ast_stmt_for_t *n) override {
    assert(n->decl);
    if (!removing_) {
      // we cant delete a loop itterator
      uses_.insert(n->decl);
    }
    ast_visitor_t::visit(n);
  }

  void visit(ast_block_t *n) override {
    if (removing_) {
      for (auto itt = n->nodes.begin(); itt != n->nodes.end();) {
        if (should_erase(*itt)) {
          itt = n->nodes.erase(itt);
        } else {
          ++itt;
        }
      }
    }
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_call_t *n) override {
    // XXX: do we need this?
    ast_visitor_t::visit(n);
  }

  void visit(ast_decl_var_t *n) override {
    decl_ = removing_ ? nullptr : n;
    ast_visitor_t::visit(n);
    decl_ = nullptr;
  }

  void visit(ast_stmt_assign_var_t *n) override {
    if (!removing_) {
      decl_ = n->decl;
      ast_visitor_t::visit(n);
      decl_ = nullptr;
    }
  }

  void visit(ast_stmt_assign_array_t *n) override {
    if (!removing_) {
      decl_ = n->decl;
      ast_visitor_t::visit(n);
      decl_ = nullptr;
    }
  }

  bool should_erase(ast_node_t *node) {
    // if this is a declaration of an unused variable
    if (auto *a = node->cast<ast_decl_var_t>()) {
      return uses_.count(a) == 0;
    }
    // if we assign to an unused variable
    if (auto *a = node->cast<ast_stmt_assign_var_t>()) {
      return uses_.count(a->decl) == 0;
    }
    // if we assign to an unused array
    if (auto *a = node->cast<ast_stmt_assign_array_t>()) {
      return uses_.count(a->decl) == 0;
    }
    return false;
  }

  void visit(ast_stmt_assign_member_t *n) {
    if (!removing_) {
      // find the decl
      const ast_decl_var_t *decl = n->decl->cast<const ast_decl_var_t>();
      assert(decl);
      if (decl) {
        uses_.insert(decl);
      }
    }
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_member_t *n) {
    if (!removing_) {
      // find the decl
      const ast_decl_var_t *decl = n->decl->cast<const ast_decl_var_t>();
      assert(decl);
      if (decl) {
        uses_.insert(decl);
      }
    }
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_ident_t *n) override {
    if (!removing_) {
      // find the decl
      const ast_decl_var_t *decl = n->decl->cast<const ast_decl_var_t>();
      // XXX: should we not assert decl?
      if (decl) {
        uses_.insert(decl);
      }
    }
    ast_visitor_t::visit(n);
  }

#if 0
  void visit(ast_exp_deref_t *n) override {
    if (!removing_) {
      // find the decl
      const ast_decl_var_t *decl = n->decl->cast<const ast_decl_var_t>();
      if (decl) {
        uses_.insert(decl);
      }
    }
    ast_visitor_t::visit(n);
  }
#endif

  void visit(ast_program_t *p) override {
    uses_.clear();
    removing_ = false;
    ast_visitor_t::visit(p);
    removing_ = true;
    ast_visitor_t::visit(p);
  }

  error_manager_t &errs_;
  ast_t &ast_;
  std::set<const ast_decl_var_t*> uses_;
  bool removing_;
  ast_decl_var_t *decl_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// constant propagation
//
struct opt_const_expr_t: public ast_visitor_t {

  opt_const_expr_t(nano_t &nano)
    : errs_(nano.errors())
    , ast_(nano.ast())
  {}

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
      v = e->val;
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

  opt_post_ret_t(nano_t &nano)
    : errs_(nano.errors())
  {}

  void visit(ast_program_t *p) override {
    ast_visitor_t::visit(p);
  }

  void visit(ast_block_t *n) override {
    assert(n);
    bool found_return = false;
    auto &nodes = n->nodes;
    for (auto i = nodes.begin(); i != nodes.end();) {
      if (found_return) {
        i = nodes.erase(i);
      }
      else {
        found_return = (*i)->is_a<ast_stmt_return_t>();
        ++i;
      }
    }
  }

protected:
  error_manager_t &errs_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// remove unreachable branches
//
struct opt_if_remove_t: public ast_visitor_t {

  opt_if_remove_t(nano_t &nano)
    : errs_(nano.errors())
  {}

  void visit(ast_stmt_if_t *s) override {
    ast_visitor_t::visit(s);
    if (auto *c = s->expr->cast<ast_exp_lit_var_t>()) {

      if (c->val != 0) {
        s->else_block = nullptr;
      }
      else {
        s->then_block = s->else_block;
        s->else_block = nullptr;
        c->val = 1;
      }
    }
  }

  void visit(ast_stmt_while_t *s) override {
    ast_visitor_t::visit(s);
    if (auto *c = s->expr->cast<ast_exp_lit_var_t>()) {
      if (c->val == 0) {
        s->body = nullptr;
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
void run_optimize(nano_t &nano) {
  if (nano.optimize) {
    opt_post_ret_t    (nano).visit(&(nano.ast().program));
    opt_const_expr_t  (nano).visit(&(nano.ast().program));
    opt_if_remove_t   (nano).visit(&(nano.ast().program));

// we have to disable this at the moment since we need to teach it that asignments
// can make values alias.
//    op_decl_elim_t    (nano).visit(&(nano.ast().program));
  }
}

} // namespace nano
