#include <set>

#include "ast.h"


namespace ccml {

void ast_visitor_t::dispatch(ast_node_t *n) {
  if (!n) {
    return;
  }
  switch (n->type) {
  case ast_program_e:           visit(n->cast<ast_program_t>());           break;
  case ast_exp_ident_e:         visit(n->cast<ast_exp_ident_t>());         break;
  case ast_exp_lit_var_e:       visit(n->cast<ast_exp_lit_var_t>());       break;
  case ast_exp_lit_str_e:       visit(n->cast<ast_exp_lit_str_t>());       break;
  case ast_exp_array_e:         visit(n->cast<ast_exp_array_t>());         break;
  case ast_exp_call_e:          visit(n->cast<ast_exp_call_t>());          break;
  case ast_exp_bin_op_e:        visit(n->cast<ast_exp_bin_op_t>());        break;
  case ast_exp_unary_op_e:      visit(n->cast<ast_exp_unary_op_t>());      break;
  case ast_stmt_if_e:           visit(n->cast<ast_stmt_if_t>());           break;
  case ast_stmt_call_e:         visit(n->cast<ast_stmt_call_t>());         break;
  case ast_stmt_while_e:        visit(n->cast<ast_stmt_while_t>());        break;
  case ast_stmt_return_e:       visit(n->cast<ast_stmt_return_t>());       break;
  case ast_stmt_assign_var_e:   visit(n->cast<ast_stmt_assign_var_t>());   break;
  case ast_stmt_assign_array_e: visit(n->cast<ast_stmt_assign_array_t>()); break;
  case ast_decl_func_e:         visit(n->cast<ast_decl_func_t>());         break;
  case ast_decl_var_e:          visit(n->cast<ast_decl_var_t>());          break;
  case ast_block_e:             visit(n->cast<ast_block_t>());             break;
  case ast_exp_none_e:          visit(n->cast<ast_exp_none_t>());          break;
  case ast_array_init_e:        visit(n->cast<ast_array_init_t>());        break;
  default:
    assert(!"unexpected ast_node_t type");
  }
}

struct gc_visitor_t: public ast_visitor_t {

  bool found(const ast_node_t *node) const {
    return nodes_.count(node) != 0;
  }

  void visit(ast_array_init_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_none_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_block_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_program_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_ident_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_lit_str_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_lit_var_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_array_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_call_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_call_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_bin_op_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_exp_unary_op_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_if_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_while_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_return_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_assign_var_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_stmt_assign_array_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_decl_func_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

  void visit(ast_decl_var_t* n) override {
    nodes_.insert(n);
    ast_visitor_t::visit(n);
  }

protected:
  std::set<const ast_node_t*> nodes_;
};

void ast_t::gc() {
  gc_visitor_t visitor;
  visitor.visit(&program);
  for (auto itt = allocs_.begin(); itt != allocs_.end();) {
    if (!visitor.found(*itt)) {
      delete *itt;
      itt = allocs_.erase(itt);
    }
    else {
      ++itt;
    }
  }
}

void ast_t::reset() {
  program.children.clear();
  for (auto *x : allocs_) {
    delete x;
  }
  allocs_.clear();
}

ast_t::~ast_t() {
  for (auto i : allocs_) {
    delete i;
  }
  allocs_.clear();
}

void ast_t::dump(FILE *fd) {
  ast_printer_t printer{fd};
  printer.visit(&program);
}

} // namespace ccml
