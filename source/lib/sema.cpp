#include <set>

#include "sema.h"
#include "ast.h"
#include "errors.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
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
  sema_multi_decls_t(ccml).visit(&(ccml.ast().program));
  sema_num_args_t(ccml).visit(&(ccml.ast().program));
  sema_type_uses_t(ccml).visit(&(ccml.ast().program));
  sema_array_size_t(ccml).visit(&(ccml.ast().program));
}

} // namespace ccml
