#include <set>

#include "ast.h"
#include "errors.h"

namespace nano {

struct eval_t: public ast_visitor_t {

  eval_t(error_manager_t &errs)
    : errs_(errs)
    , valid_(false)
  {
  }

  void visit(ast_exp_unary_op_t *n) override {

    if (!is_supported(n->child)) {
      valid_ = false;
      return;
    }

    decend_(n);
    const int32_t v = value_.back();
    int32_t r = 0;
    value_.pop_back();
    switch (n->op->type_) {
    case TOK_SUB:
      r = -v;
      break;
    default:
      assert(!"unknown operator");
    }
    value_.push_back(r);
  }

  void visit(ast_exp_bin_op_t *n) override {

    if (!is_supported(n->left) || !is_supported(n->right)) {
      valid_ = false;
      return;
    }

    decend_(n);
    const int32_t r = eval_(*(n->token));
    value_.push_back(r);
  }

  void visit(ast_exp_lit_var_t *n) override {
    value_.push_back(n->val);
  }

  void visit(ast_exp_ident_t *n) override {
    assert(n->decl);
    ast_decl_var_t *decl = n->decl->cast<ast_decl_var_t>();
    assert(decl);
    if (decl->is_const) {
      if (auto *v = decl->expr->cast<ast_exp_lit_var_t>()) {
        value_.push_back(v->val);
        return;
      }
    }
    valid_ = false;
  }

  bool eval(ast_node_t *node, int32_t &result) {
    valid_ = true;
    dispatch(node);
    valid_ &= (value_.size() == 1);
    if (valid_) {
      result = value_.front();
    }
    return valid_;
  }

protected:
  bool is_supported(const ast_node_t *n) const {
    switch (n->type) {
    case ast_exp_lit_var_e:
    case ast_exp_bin_op_e:
    case ast_exp_unary_op_e:
    case ast_exp_ident_e:
      return true;
    default:
      return false;
    }
  }

  void decend_(ast_node_t *n) {
    switch (n->type) {
    case ast_exp_lit_var_e:
      ast_visitor_t::visit(n->cast<ast_exp_lit_var_t>());
      break;
    case ast_exp_ident_e:
      ast_visitor_t::visit(n->cast<ast_exp_ident_t>());
      break;
    case ast_exp_bin_op_e:
      ast_visitor_t::visit(n->cast<ast_exp_bin_op_t>());
      break;
    case ast_exp_unary_op_e:
      ast_visitor_t::visit(n->cast<ast_exp_unary_op_t>());
      break;
    default:
      valid_ = false;
    }
  }

  int32_t eval_(const token_t &tok) {
    if (valid_ == false) {
      return 0;
    }
    assert(value_.size() >= 2);
    const int32_t b = value_.back();
    value_.pop_back();
    const int32_t a = value_.back();
    value_.pop_back();
    // check for constant division
    if (b == 0) {
      if (tok.type_ == TOK_DIV || tok.type_ == TOK_MOD) {
#if 1
        errs_.constant_divie_by_zero(tok);
#else
        valid_ = false;
        return -1;
#endif
      }
    }
    // evaluate operator
    switch (tok.type_) {
    case TOK_ADD:     return a +  b;
    case TOK_SUB:     return a -  b;
    case TOK_MUL:     return a *  b;
    case TOK_AND:     return a && b;
    case TOK_OR:      return a || b;
    case TOK_LEQ:     return a <= b;
    case TOK_GEQ:     return a >= b;
    case TOK_LT:      return a <  b;
    case TOK_GT:      return a >  b;
    case TOK_EQ:      return a == b;
    case TOK_DIV:     return a /  b;
    case TOK_MOD:     return a %  b;
    default:
      assert(!"unknown operator");
    }
    return 0;
  }

  std::vector<int32_t> value_;
  error_manager_t &errs_;
  bool valid_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// enforce constants are read only and inlined where used
//
struct sema_const_t: public ast_visitor_t {

  sema_const_t(nano_t &nano)
    : errs_(nano.errors())
  {
  }

  void visit(ast_exp_ident_t *n) override {
    ast_decl_var_t *decl = n->decl->cast<ast_decl_var_t>();
    if (!decl) {
      return;
    }
    if (!decl->is_const) {
      return;
    }
    ast_node_t *parent = stack.rbegin()[1];
    if (decl->expr) {
      ast_node_t *e = decl->expr;
      switch (e->type) {
      case ast_exp_lit_var_e:
      case ast_exp_lit_str_e:
      case ast_exp_lit_float_e:
      case ast_exp_none_e:
        parent->replace_child(n, e);
        break;
      default:
        errs_.cant_evaluate_constant(*n->name);
      }
    }
  }

  void visit(ast_stmt_assign_var_t *n) override {
    assert(n->decl);
    if (n->decl->is_const) {
      errs_.cant_assign_const(*n->name);
    }
    ast_visitor_t::visit(n);
  }

  void visit(ast_decl_var_t *n) override {
    ast_visitor_t::visit(n);
    if (n->is_const) {
      if (n->is_array()) {
        errs_.const_array_invalid(*n->name);
      }
      if (!n->expr) {
        errs_.const_needs_init(*n->name);
      }
    }
  }

  void visit(ast_program_t *p) override {
    ast_visitor_t::visit(p);
  }

  error_manager_t &errs_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// check global var expressions
//
// this does constant propagation
//
struct sema_global_var_t : public ast_visitor_t {

  sema_global_var_t(nano_t &nano)
    : errs_(nano.errors())
    , ast_(nano.ast())
    , change_(false)
    , strict_(false) {
  }

  void visit(ast_array_init_t *n) override {
    for (auto *t : n->item) {
      switch (t->type_) {
      case TOK_INT:
      case TOK_NONE:
      case TOK_STRING:
      case TOK_FLOAT:
        break;
      default:
        errs_.bad_array_init_value(*t);
      }
    }
  }

  void visit(ast_decl_var_t *n) override {

    eval_t eval(errs_);

    assert(n->expr);

    switch (n->expr->type) {
    case ast_exp_none_e:
      n->expr = nullptr;
      break;
    case ast_array_init_e:
      break;
    case ast_exp_bin_op_e:
    case ast_exp_unary_op_e: {
      int32_t val = 0;
      if (eval.eval(n->expr, val)) {
        change_ = true;
        n->expr = ast_.alloc<ast_exp_lit_var_t>(val);
      }
      else {
        if (strict_) {
          errs_.global_var_const_expr(*n->name);
        }
      }
    } break;
    case ast_exp_lit_str_e:
    case ast_exp_lit_var_e:
    case ast_exp_lit_float_e:
      break;
    default:
      errs_.global_var_const_expr(*n->name);
    }
  }

  void visit(ast_program_t *p) override {
    strict_ = false;
    do {
      change_ = false;
      run_globals_(p);
    } while (change_);
    // at this point raise any errors that were not resolved
    strict_ = true;
    run_globals_(p);
  }

protected:
  void run_globals_(ast_program_t *p) {
    for (auto *n : p->children) {
      if (auto *d = n->cast<ast_decl_var_t>()) {
        if (d->expr) {
          dispatch(d);
        }
      }
    }
  }

  error_manager_t &errs_;
  ast_t &ast_;
  bool change_;
  bool strict_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// annotate nodes with their associated decl node
//
struct sema_decl_annotate_t : public ast_visitor_t {

  sema_decl_annotate_t(nano_t &nano)
    : errs_(nano.errors())
    , nano_(nano)
    , prog_(nullptr) {
  }

  void visit(ast_stmt_for_t *p) override {
    scope_.emplace_back();
    ast_node_t *n = find_decl(p->name->str_);
    if (n) {
      if (ast_decl_var_t *d = n->cast<ast_decl_var_t>()) {
        p->decl = d;
        ast_visitor_t::visit(p);
        scope_.pop_back();
        return;
      }
    }
    errs_.unknown_variable(*p->name);
  }

  void visit(ast_program_t *p) override {
    // collect global decls
    scope_.emplace_back();
    for (auto &f : p->children) {
      switch (f->type) {
      case ast_decl_func_e:
      case ast_decl_var_e:
        scope_[0].insert(f);
        break;
      default:
        break;
      }
    }
    prog_ = p;
    ast_visitor_t::visit(p);
  }

  void visit(ast_stmt_assign_var_t *n) override {
    ast_visitor_t::visit(n);
    ast_node_t *found = find_decl(n->name->str_);
    if (!found) {
      errs_.unknown_variable(*n->name);
    }
    n->decl = found->cast<ast_decl_var_t>();
    if (!n->decl) {
      errs_.unknown_variable(*n->name);
    }
    if (n->decl->is_array()) {
      errs_.ident_is_array_not_var(*n->name);
    }
  }

  void visit(ast_stmt_assign_array_t *n) override {
    ast_visitor_t::visit(n);
    ast_node_t *found = find_decl(n->name->str_);
    if (!found) {
      errs_.unknown_array(*n->name);
    }
    n->decl = found->cast<ast_decl_var_t>();
    if (!n->decl) {
      errs_.unknown_array(*n->name);
    }
  }

  void visit(ast_stmt_assign_member_t *n) override {
    ast_visitor_t::visit(n);
    ast_node_t *found = find_decl(n->name->str_);
    if (!found) {
      errs_.unknown_identifier(*n->name);
    }
    n->decl = found->cast<ast_decl_var_t>();
    if (!n->decl) {
      errs_.unknown_identifier(*n->name);
    }
  }

  void visit(ast_exp_array_t *n) override {
    ast_visitor_t::visit(n);
    ast_node_t *found = find_decl(n->name->str_);
    if (!found) {
      errs_.unknown_array(*n->name);
    }
    n->decl = found->cast<ast_decl_var_t>();
    if (n->decl) {
      if (!n->decl->is_array()) {
        // runtime error as strings are also acceptable
        // errs_.variable_is_not_array(*n->name);
      }
      return;
    } else {
      if (found->cast<ast_decl_func_t>()) {
        errs_.expected_func_call(*n->name);
      } else {
        errs_.unexpected_token((*n->name));
      }
    }
  }

  void visit(ast_exp_member_t *n) override {
    ast_visitor_t::visit(n);
    n->decl = find_decl(n->name->str_);
    if (n->decl) {
      if (!n->decl->cast<ast_decl_var_t>()) {
        assert(!"TODO: Revise this later please");
      }
      return;
    }
    errs_.unknown_identifier(*n->name);
  }

  void visit(ast_exp_ident_t *n) override {
    ast_visitor_t::visit(n);
    assert(!n->decl);
    n->decl = find_decl(n->name->str_);
    if (!n->decl) {
      errs_.unknown_identifier(*n->name);
    }
    if (const auto *v = n->decl->cast<ast_decl_var_t>()) {
      if (v->is_array()) {
        errs_.array_requires_subscript(*n->name);
      }
    } else {
      if (n->decl->cast<ast_decl_func_t>()) {
        // this is fine
      } else {
        errs_.unexpected_token((*n->name));
      }
    }
  }

  void visit(ast_exp_call_t *n) override {
    ast_visitor_t::visit(n);
  }

  void visit(ast_decl_func_t *n) override {
    scope_.emplace_back();
    ast_visitor_t::visit(n);
    scope_.pop_back();
  }

  void visit(ast_stmt_if_t *n) override {
    scope_.emplace_back();
    ast_visitor_t::visit(n);
    scope_.pop_back();
  }

  void visit(ast_stmt_while_t *n) override {
    scope_.emplace_back();
    ast_visitor_t::visit(n);
    scope_.pop_back();
  }

  void visit(ast_decl_var_t *n) override {
    ast_visitor_t::visit(n);
    scope_.back().insert(n);
  }

  ast_node_t *find_decl(const std::string &name) {
    ast_node_t *out = nullptr;
    // move from inner to outer scope
    for (auto s = scope_.rbegin(); s != scope_.rend(); ++s) {
      for (auto d : *s) {
        if (auto *m = d->cast<ast_decl_func_t>()) {
          if (m->name.c_str() == name)
            return m;
          else
            continue;
        }
        if (auto *m = d->cast<ast_decl_var_t>()) {
          if (m->name->str_ == name)
            return m;
          else
            continue;
        }
      }
    }
    return out;
  }

  std::vector<std::set<ast_node_t *>> scope_;

  error_manager_t &errs_;
  nano_t &nano_;
  ast_program_t *prog_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// check for multiple declarations with the same name
//
struct sema_multi_decls_t : public ast_visitor_t {

  sema_multi_decls_t(nano_t &nano)
    : errs_(nano.errors()) {
  }

  void visit(ast_decl_var_t *var) override {
    const auto &name = var->name->str_;
    if (is_def_(name)) {
      errs_.var_already_exists(*var->name);
    }
    add_(name);
  }

  void visit(ast_stmt_if_t *stmt) override {
    dispatch(stmt->expr);
    if (stmt->then_block) {
      scope_.emplace_back();
      dispatch(stmt->then_block);
      scope_.pop_back();
    }
    if (stmt->else_block) {
      scope_.emplace_back();
      dispatch(stmt->else_block);
      scope_.pop_back();
    }
  }

  void visit(ast_stmt_while_t *stmt) override {
    dispatch(stmt->expr);
    scope_.emplace_back();
    dispatch(stmt->body);
    scope_.pop_back();
  }

  void visit(ast_stmt_for_t *stmt) override {
    dispatch(stmt->start);
    dispatch(stmt->end);
    scope_.emplace_back();
    dispatch(stmt->body);
    scope_.pop_back();
  }

  void visit(ast_decl_func_t *func) override {
    if (is_def_(func->name.c_str())) {
      errs_.function_already_exists(*func->token);
    }
    add_(func->name.c_str());
    scope_.emplace_back();
    for (const auto &a : func->args) {
      dispatch(a);
    }
    dispatch(func->body);
    scope_.pop_back();
  }

  void visit(ast_program_t *prog) override {
    scope_.emplace_back();
    ast_visitor_t::visit(prog);
    scope_.pop_back();
  }

protected:
  error_manager_t &errs_;
  std::vector<std::set<std::string>> scope_;

  // add a name to scope
  void add_(const std::string &name) {
    scope_.back().insert(name);
  }

  // check if a name is already present
  bool is_def_(const std::string &name) const {
    for (const auto &s : scope_) {
      for (const auto &n : s) {
        if (n == name) {
          return true;
        }
      }
    }
    return false;
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// check for valid/compatable type usage for certain operations
//
struct sema_type_uses_t : public ast_visitor_t {

  sema_type_uses_t(nano_t &nano)
    : errs_(nano.errors()) {
  }

  void visit(ast_decl_var_t *var) override {
    add_(var);
  }

  void visit(ast_stmt_assign_array_t *a) override {
    const ast_decl_var_t *d = get_decl_(a->name->str_);
    if (d && !d->is_array()) {
      errs_.variable_is_not_array(*d->name);
    }
  }

  void visit(ast_stmt_assign_var_t *v) override {
    const ast_decl_var_t *d = get_decl_(v->name->str_);
    if (d && d->is_array()) {
      errs_.ident_is_array_not_var(*d->name);
    }
  }

  void visit(ast_exp_ident_t *i) override {
    const ast_decl_var_t *d = get_decl_(i->name->str_);
    if (d && d->is_array()) {
      errs_.ident_is_array_not_var(*d->name);
    }
  }

  void visit(ast_stmt_if_t *stmt) override {
    scope_.emplace_back();
    ast_visitor_t::visit(stmt);
    scope_.pop_back();
  }

  void visit(ast_stmt_while_t *stmt) override {
    scope_.emplace_back();
    ast_visitor_t::visit(stmt);
    scope_.pop_back();
  }

  void visit(ast_decl_func_t *func) override {
    scope_.emplace_back();
    ast_visitor_t::visit(func);
    scope_.pop_back();
  }

  void visit(ast_program_t *prog) override {
    scope_.emplace_back();
    ast_visitor_t::visit(prog);
    scope_.pop_back();
  }

protected:
  error_manager_t &errs_;
  std::vector<std::set<const ast_decl_var_t *>> scope_;

  void add_(const ast_decl_var_t *v) {
    scope_.back().insert(v);
  }

  const ast_decl_var_t *get_decl_(const std::string &name) const {
    for (const auto &s : scope_) {
      for (const auto &n : s) {
        if (n->name->str_ == name) {
          return n;
        }
      }
    }
    return nullptr;
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// check call sites use the correct number of arguments
//
struct sema_num_args_t : public ast_visitor_t {

  sema_num_args_t(nano_t &nano)
    : nano_(nano)
    , errs_(nano.errors()) {
  }

  nano_t &nano_;
  error_manager_t &errs_;
  std::map<std::string, const ast_decl_func_t *> funcs_;

  void visit(ast_exp_call_t *call) override {
    assert(call->callee);
    // if this can be lowered to a direct call
    if (ast_exp_ident_t *ident = call->callee->cast<ast_exp_ident_t>()) {
      if (ast_decl_func_t *callee = ident->decl->cast<ast_decl_func_t>()) {
        if (callee->is_syscall && callee->is_varargs) {
          // its varargs so lets make an exception here
        }
        else {
          if (call->args.size() > callee->args.size()) {
            errs_.too_many_args(*ident->name);
          }
          if (call->args.size() < callee->args.size()) {
            errs_.not_enought_args(*ident->name);
          }
        }
      }
    }
    // dispatch
    ast_visitor_t::visit(call);
  }

  void visit(ast_program_t *prog) override {
    // gather all functions
    for (ast_node_t *n : prog->children) {
      if (auto *f = n->cast<ast_decl_func_t>()) {
        const std::string &name = f->name.c_str();
        funcs_.emplace(name, f);
      }
    }
    // dispatch
    ast_visitor_t::visit(prog);
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// check for a valid array size
//
struct sema_array_size_t : public ast_visitor_t {

  error_manager_t &errs_;
  ast_t &ast_;

  sema_array_size_t(nano_t &nano)
    : errs_(nano.errors())
    , ast_(nano.ast()) {
  }

  void visit(ast_decl_var_t *d) override {
    if (!d->is_array()) {
      return;
    }
    // size should be valid
    if (d->size == nullptr) {
      assert(!"ast_decl_var_t array needs size");
    }

    // ensure size is const expr
    eval_t eval{errs_};
    int32_t res = 0;
    if (!eval.eval(d->size, res)) {
      errs_.global_var_const_expr(*d->name);
    }

    // size should be resolved as literal int now
    if (!d->size->is_a<ast_exp_lit_var_t>()) {
      d->size = ast_.alloc<ast_exp_lit_var_t>(res);
    }

    // size should be a valid count
    if (d->count() <= 1) {
      errs_.array_size_must_be_greater_than(*d->name);
    }
    if (d->expr) {
      if (auto i = d->expr->cast<ast_array_init_t>()) {
        const int32_t space = d->count();
        const int32_t inits = int32_t(i->item.size());
        if (space < inits) {
          errs_.too_many_array_inits(*d->name, inits, space);
        }
      }
    }
  }

  void visit(ast_program_t *p) override {
    ast_visitor_t::visit(p);
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void run_sema(nano_t &nano) {
  auto *prog = &(nano.ast().program);
  sema_decl_annotate_t(nano).visit(prog);
  sema_global_var_t(nano).visit(prog);
  sema_const_t(nano).visit(prog);
  sema_multi_decls_t(nano).visit(prog);
  sema_num_args_t(nano).visit(prog);
  sema_type_uses_t(nano).visit(prog);
  sema_array_size_t(nano).visit(prog);
}

} // namespace nano
