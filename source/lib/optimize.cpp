#include <set>

#include "sema.h"
#include "ast.h"
#include "errors.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
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
void run_optimize(ccml_t &ccml) {
  opt_post_ret_t(ccml).visit(&(ccml.ast().program));
}

} // namespace ccml
