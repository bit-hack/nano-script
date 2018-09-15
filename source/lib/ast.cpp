#include "ast.h"

namespace ccml {

void ast_visitor_t::dispatch(ast_node_t *n) {
  if (!n) {
    return;
  }
  switch (n->type) {
  case ast_program_e:           visit(n->cast<ast_program_t>());           break;
  case ast_exp_ident_e:         visit(n->cast<ast_exp_ident_t>());         break;
  case ast_exp_const_e:         visit(n->cast<ast_exp_const_t>());         break;
  case ast_exp_array_e:         visit(n->cast<ast_exp_array_t>());         break;
  case ast_exp_call_e:          visit(n->cast<ast_exp_call_t>());          break;
  case ast_exp_bin_op_e:        visit(n->cast<ast_exp_bin_op_t>());        break;
  case ast_exp_unary_op_e:      visit(n->cast<ast_exp_unary_op_t>());      break;
  case ast_stmt_if_e:           visit(n->cast<ast_stmt_if_t>());           break;
  case ast_stmt_while_e:        visit(n->cast<ast_stmt_while_t>());        break;
  case ast_stmt_return_e:       visit(n->cast<ast_stmt_return_t>());       break;
  case ast_stmt_assign_var_e:   visit(n->cast<ast_stmt_assign_var_t>());   break;
  case ast_stmt_assign_array_e: visit(n->cast<ast_stmt_assign_array_t>()); break;
  case ast_decl_func_e:         visit(n->cast<ast_decl_func_t>());         break;
  case ast_decl_var_e:          visit(n->cast<ast_decl_var_t>());          break;
  case ast_decl_array_e:        visit(n->cast<ast_decl_array_t>());        break;
  default:
    assert(!"unexpected ast_node_t type");
  }
}

} // namespace ccml
