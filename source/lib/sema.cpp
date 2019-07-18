#include <set>

#include "sema.h"
#include "ast.h"
#include "errors.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// annotate nodes with a data type where possible to check validity
//
struct sema_type_annotation_t: public ast_visitor_t {

  sema_type_annotation_t(ccml_t &ccml)
    : errs_(ccml.errors()) {}

  std::map<const ast_node_t*, value_type_t> types_;

  value_type_t type_of(const ast_node_t *n) const {
    assert(n);
    auto itt = types_.find(n);
    if (itt != types_.end()) {
      return itt->second;
    }
    else {
      return val_type_unknown;
    }
  }

  void visit(ast_exp_lit_var_t *node) override {
    types_[node] = val_type_int;
  }

  void visit(ast_exp_lit_str_t *node) override {
    types_[node] = val_type_string;
  }

  void visit(ast_exp_none_t *node) override {
    types_[node] = val_type_none;
  }

  void visit(ast_exp_ident_t* n) override {
    ast_visitor_t::visit(n);
    if (n->decl) {
      auto itt = types_.find(n->decl);
      if (itt != types_.end()) {
        types_[n] = itt->second;
      }
    }
  }

  void visit(ast_exp_array_t* n) override {
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_call_t* n) override {
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_bin_op_t* n) override {
    ast_visitor_t::visit(n);
    assert(n->left && n->right);
    auto tl = type_of(n->left);
    auto tr = type_of(n->right);

    if (tl == val_type_int &&
        tr == val_type_int) {
      types_[n] = val_type_int;
      return;
    }

    if (n->op == TOK_ADD) {
      if (tl == val_type_string && tr == val_type_string) {
        types_[n] = val_type_string;
        return;
      }
      if (tl == val_type_string && tr == val_type_int) {
        types_[n] = val_type_string;
        return;
      }
      if (tl == val_type_int && tr == val_type_string) {
        types_[n] = val_type_string;
        return;
      }
    }
  }

  void visit(ast_exp_unary_op_t* n) override {
    ast_visitor_t::visit(n);

    if (type_of(n->child) == val_type_int) {
      types_[n] = val_type_int;
    } else {
      // we cant unary a string
    }
  }

  void visit(ast_decl_var_t* n) override {
    ast_visitor_t::visit(n);
    if (n->expr) {
      switch (type_of(n->expr)) {
      case val_type_int:
        break;
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
// annotate nodes with their associated decl node
// set function `is_syscall` member
//
struct sema_decl_annotate_t: public ast_visitor_t {

  sema_decl_annotate_t(ccml_t &ccml)
    : errs_(ccml.errors()) 
    , ccml_(ccml)
  {}

  void visit(ast_program_t *p) override {

    // collect global decls
    scope_.emplace_back();
    for (auto &f : p->children) {
      switch (f->type) {
      case ast_decl_func_e:
      case ast_decl_var_e:
        scope_[0].insert(f);
      }
    }

    prog_ = p;
    ast_visitor_t::visit(p);
  }

  void visit(ast_stmt_assign_var_t* n) override {
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
    if (n->decl->is_arg()) {
      // can you assign to an argument?
    }
  }

  void visit(ast_stmt_assign_array_t* n) override {
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

  void visit(ast_exp_array_t* n) override {
    ast_visitor_t::visit(n);
    ast_node_t *found = find_decl(n->name->str_);
    if (!found) {
      errs_.unknown_array(*n->name);
    }
    n->decl = found->cast<ast_decl_var_t>();
    if (n->decl) {
      if (!n->decl->is_array()) {
        errs_.variable_is_not_array(*n->name);
      }
    }
    if (n->decl->cast<ast_decl_func_t>()) {
      errs_.expected_func_call(*n->name);
    }
  }

  void visit(ast_exp_ident_t* n) override {
    ast_visitor_t::visit(n);
    n->decl = find_decl(n->name->str_);
    if (!n->decl) {
      errs_.unknown_identifier(*n->name);
    }
    if (const auto *v = n->decl->cast<ast_decl_var_t>()) {
      if (v->is_array()) {
        errs_.array_requires_subscript(*n->name);
      }
    }
    if (n->decl->cast<ast_decl_func_t>()) {
      errs_.expected_func_call(*n->name);
    }
  }

  void visit(ast_exp_call_t* n) override {
    ast_visitor_t::visit(n);
    ast_node_t *f = find_decl(n->name->str_);
    if (f) {
      n->decl = f->cast<ast_decl_func_t>();
      if (n->decl) {
        n->is_syscall = false;
        return;
      }
    }
    else {
      // check for extern function
      for (const auto &func : ccml_.functions()) {
        if (func.name_ == n->name->str_) {
          n->is_syscall = true;
          return;
        }
      }
    }
    errs_.unknown_function(*n->name);
  }

  void visit(ast_decl_func_t* n) override {
    scope_.emplace_back();
    ast_visitor_t::visit(n);
    scope_.pop_back();
  }

  void visit(ast_stmt_if_t* n) override {
    scope_.emplace_back();
    ast_visitor_t::visit(n);
    scope_.pop_back();
  }

  void visit(ast_stmt_while_t* n) override {
    scope_.emplace_back();
    ast_visitor_t::visit(n);
    scope_.pop_back();
  }

  void visit(ast_decl_var_t* n) override {
    ast_visitor_t::visit(n);
    scope_.back().insert(n);
  }

  ast_node_t* find_decl(const std::string &name) {
    // move from inner to outer scope
    for (auto s = scope_.rbegin(); s != scope_.rend(); ++s) {
      for (auto d : *s) {
        if (auto *m = d->cast<ast_decl_func_t>()) {
          if (m->name->str_ == name)
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
    return nullptr;
  }

  std::vector<std::set<ast_node_t*>> scope_;

  ast_program_t *prog_;
  ccml_t &ccml_;
  error_manager_t &errs_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// check for multiple declarations with the same name
//
struct sema_multi_decls_t: public ast_visitor_t {

  sema_multi_decls_t(ccml_t &ccml)
    : errs_(ccml.errors()) {
  }

  void visit(ast_decl_var_t *var) {
    const auto &name = var->name->str_;
    if (is_def_(name)) {
      errs_.var_already_exists(*var->name);
    }
    add_(name);
  }

  void visit(ast_stmt_if_t *stmt) {
    scope_.emplace_back();
    ast_visitor_t::visit(stmt);
    scope_.pop_back();
  }

  void visit(ast_stmt_while_t *stmt) {
    scope_.emplace_back();
    ast_visitor_t::visit(stmt);
    scope_.pop_back();
  }

  void visit(ast_decl_func_t *func) {
    if (is_def_(func->name->str_)) {
      errs_.function_already_exists(*func->name);
    }
    add_(func->name->str_);
    scope_.emplace_back();
    for (const auto &a : func->args) {
      dispatch(a);
    }
    for (const auto &s : func->body) {
      dispatch(s);
    }
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
struct sema_type_uses_t: public ast_visitor_t {

  sema_type_uses_t(ccml_t &ccml)
    : errs_(ccml.errors()) {
  }

  void visit(ast_decl_var_t *var) {
    add_(var);
  }

  void visit(ast_stmt_assign_array_t *a) {
    const ast_decl_var_t *d = get_decl_(a->name->str_);
    if (d && !d->is_array()) {
      errs_.variable_is_not_array(*d->name);
    }
  }

  void visit(ast_stmt_assign_var_t *v) {
    const ast_decl_var_t *d = get_decl_(v->name->str_);
    if (d && d->is_array()) {
      errs_.ident_is_array_not_var(*d->name);
    }
  }

  void visit(ast_exp_array_t *i) {
    const ast_decl_var_t *d = get_decl_(i->name->str_);
    if (d && !d->is_array()) {
      errs_.variable_is_not_array(*d->name);
    }
  }

  void visit(ast_exp_ident_t *i) {
    const ast_decl_var_t *d = get_decl_(i->name->str_);
    if (d && d->is_array()) {
      errs_.ident_is_array_not_var(*d->name);
    }
  }

  void visit(ast_stmt_if_t *stmt) {
    scope_.emplace_back();
    ast_visitor_t::visit(stmt);
    scope_.pop_back();
  }

  void visit(ast_stmt_while_t *stmt) {
    scope_.emplace_back();
    ast_visitor_t::visit(stmt);
    scope_.pop_back();
  }

  void visit(ast_decl_func_t *func) {
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
  std::vector<std::set<const ast_decl_var_t*>> scope_;

  void add_(const ast_decl_var_t *v) {
    scope_.back().insert(v);
  }

  const ast_decl_var_t* get_decl_(const std::string &name) const {
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
struct sema_num_args_t: public ast_visitor_t {

  sema_num_args_t(ccml_t &ccml)
    : errs_(ccml.errors()) {}

  error_manager_t &errs_;
  std::map<std::string, const ast_decl_func_t*> funcs_;

  void visit(ast_exp_call_t *call) override {
    const auto &name = call->name->str_;
    auto itt = funcs_.find(name);
    if (itt == funcs_.end()) {
      // its likely a builtin or syscall
      return;
    }
    const ast_decl_func_t *func = itt->second;
    if (call->args.size() > func->args.size()) {
      errs_.too_many_args(*call->name);
    }
    if (call->args.size() < func->args.size()) {
      errs_.not_enought_args(*call->name);
    }
  }

  void visit(ast_program_t *prog) override {
    // gather all functions
    for (ast_node_t *n : prog->children) {
      if (auto *f = n->cast<ast_decl_func_t>()) {
        const std::string &name = f->name->str_;
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
struct sema_array_size_t: public ast_visitor_t {

  sema_array_size_t(ccml_t &ccml)
    : errs_(ccml.errors()) {}

  error_manager_t &errs_;

  void visit(ast_decl_var_t *d) override {
    if (d->is_array()) {
      if (d->size->val_ <= 1) {
        errs_.array_size_must_be_greater_than(*d->name);
      }
    }
  }

  void visit(ast_program_t *p) override {
    ast_visitor_t::visit(p);
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void run_sema(ccml_t &ccml) {
  sema_decl_annotate_t(ccml).visit(&(ccml.ast().program));
  sema_type_annotation_t(ccml).visit(&(ccml.ast().program));
  sema_multi_decls_t(ccml).visit(&(ccml.ast().program));
  sema_num_args_t(ccml).visit(&(ccml.ast().program));
  sema_type_uses_t(ccml).visit(&(ccml.ast().program));
  sema_array_size_t(ccml).visit(&(ccml.ast().program));
}

} // namespace ccml
