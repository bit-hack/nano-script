#include "parser.h"
#include "codegen.h"
#include "lexer.h"
#include "token.h"
#include "errors.h"
#include "ast.h"
#include "instructions.h"


using namespace nano;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
bool parser_t::parse(error_t &error) {
  token_stream_t &stream_ = nano_.lexer().stream();

  ast_program_t *program = &nano_.ast().program;

  // format:
  //    var <TOK_IDENT> [ = <TOK_INT> ]
  //    function <TOK_IDENT> ( [ <TOK_IDENT> [ , <TOK_IDENT> ]+ ] )

  try {

    while (!stream_.found(TOK_EOF)) {

      const token_t *t = stream_.pop();
      switch (t->type_) {
      case TOK_EOL:
        // consume any blank lines
        continue;
      case TOK_VAR: {
        ast_node_t *node = parse_global_(*t);
        assert(node);
        program->children.push_back(node);
        continue;
      }
      case TOK_CONST: {
        ast_node_t *node = parse_const_(*t);
        assert(node);
        program->children.push_back(node);
        continue;
      }
      case TOK_FUNC: {
        ast_node_t *func = parse_function_(*t);
        assert(func);
        program->children.push_back(func);
        continue;
      }
      case TOK_IMPORT: {
        parse_import_(*t);
        continue;
      }
      default:
        break;
      }
      const token_t *tok = stream_.pop();
      assert(tok);
      nano_.errors().unexpected_token(*tok);
    }
  } catch (const error_t &e) {
    error = e;
    return false;
  }

  return true;
}

bool parser_t::parse_import_(const token_t &t) {
  (void)t;
  token_stream_t &stream_ = nano_.lexer().stream();
  const token_t *s = stream_.pop(TOK_STRING);
  assert(nano_.sources_ && nano_.source_);
  std::string path = s->string();
  // convert to a relative path
  nano_.sources_->imported_path(*nano_.source_, path);
  if (!nano_.sources_->load(path.c_str())) {
    nano_.errors().bad_import(*s);
    return false;
  }
  return true;
}

int32_t parser_t::op_type_(token_e type) const {
  // return precedence
  // note: higher number will be evaluated earlier in an expression
  switch (type) {
  case TOK_AND:
  case TOK_OR:
    return 1;
  case TOK_NOT:
    return 2;
  case TOK_LT:
  case TOK_GT:
  case TOK_LEQ:
  case TOK_GEQ:
  case TOK_EQ:
    return 3;
  case TOK_ADD:
  case TOK_SUB:
    return 4;
  case TOK_MUL:
  case TOK_DIV:
  case TOK_MOD:
    return 5;
  default:
    // this is not an operator
    return 0;
  }
}

bool parser_t::is_operator_() const {
  token_stream_t &stream_ = nano_.lexer().stream();
  return op_type_(stream_.type()) > 0;
}

void parser_t::parse_lhs_() {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //    ( <expr> )
  //    <TOK_IDENT>
  //    <TOK_IDENT> [ ... ]
  //    <TOK_INT>
  //    <TOK_FLOAT>
  //    <TOK_STRING>
  //    <TOK_NONE>

  do {

    // ( <expr> )
    if (stream_.found(TOK_LPAREN)) {
      ast_node_t *expr = parse_expr_();
      exp_stack_.push_back(expr);
      stream_.found(TOK_RPAREN);
      break;
    }

    if (const token_t *t = stream_.found(TOK_IDENT)) {

      // <TOK_IDENT> [ ... ]
      if (stream_.found(TOK_LBRACKET)) {

        // XXX: move out of here

        // array access
        ast_exp_array_t *expr = ast.alloc<ast_exp_array_t>(t);
        expr->index = parse_expr_();
        exp_stack_.push_back(expr);
        // expect a closing bracket
        stream_.pop(TOK_RBRACKET);
        break;

      // <TOK_IDENT>
      } else {
        // load a local/global
        ast_node_t *expr = ast.alloc<ast_exp_ident_t>(t);
        exp_stack_.push_back(expr);
        break;
      }
    }

    // <TOK_INT>
    if (const token_t *t = stream_.found(TOK_INT)) {
      ast_node_t *expr = ast.alloc<ast_exp_lit_var_t>(t);
      exp_stack_.push_back(expr);
      break;
    }

    // <TOK_FLOAT>
    if (const token_t *t = stream_.found(TOK_FLOAT)) {
      ast_node_t *expr = ast.alloc<ast_exp_lit_float_t>(t);
      exp_stack_.push_back(expr);
      break;
    }

    // <TOK_STRING>
    if (const token_t *t = stream_.found(TOK_STRING)) {
      ast_node_t *expr = ast.alloc<ast_exp_lit_str_t>(t);
      exp_stack_.push_back(expr);
      break;
    }

    // <TOK_NONE>
    if (const token_t *t = stream_.found(TOK_NONE)) {
      ast_node_t *expr = ast.alloc<ast_exp_none_t>(t);
      exp_stack_.push_back(expr);
      break;
    }

    nano_.errors().expecting_lit_or_ident(*stream_.pop());

  } while (false);
}

void parser_t::parse_expr_ex_(uint32_t tide) {
  token_stream_t &stream_ = nano_.lexer().stream();

  // format:
  //    ['not'] [-] <lhs> [ '(' <expr> ')' ]
  //    ['not'] [-] <lhs> [ '(' <expr> ')' ] <op> <expr_ex>

  // not
  if (const token_t *n = stream_.found(TOK_NOT)) {
    parse_expr_ex_(tide);
    op_push_(n, tide);
    return;
  }

  // unary minus
  if (const token_t *n = stream_.found(TOK_SUB)) {
    parse_expr_ex_(tide);
    ast_node_t *back = exp_stack_.back();
    exp_stack_.pop_back();
    auto *op = nano_.ast().alloc<ast_exp_unary_op_t>(n);
    op->child = back;
    exp_stack_.push_back(op);
    return;
  }

  parse_lhs_();

  while (true) {

    // XXX: parse [] here

    // <EXPR> ( ... )
    if (const token_t *t = stream_.found(TOK_LPAREN)) {
      // call function
      ast_exp_call_t *call = parse_call_(*t);
      // pop the callee
      assert(!exp_stack_.empty());
      call->callee = exp_stack_.back();
      exp_stack_.pop_back();
      // push the call
      exp_stack_.push_back(call);
      continue;
    }

    break;
  }

  if (is_operator_()) {
    const token_t *op = stream_.pop();
    op_push_(op, tide);
    parse_expr_ex_(tide);
  }
}

ast_node_t* parser_t::parse_expr_() {

  // format:
  //    <expr_ex>

  const uint32_t tide = op_stack_.size();
  parse_expr_ex_(tide);
  op_pop_all_(tide);

  assert(!exp_stack_.empty());
  ast_node_t *out = exp_stack_.back();
  exp_stack_.pop_back();

  return out;
}

ast_node_t* parser_t::parse_decl_array_(const token_t &name) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //                        V
  //    var <TOK_IDENT> '['   <expr> ']'

  auto *decl = ast.alloc<ast_decl_var_t>(&name, ast_decl_var_t::e_local);

  decl->size = parse_expr_();

  stream_.pop(TOK_RBRACKET);

  // initalization
  if (stream_.found(TOK_ASSIGN)) {

    auto *init = ast.alloc<ast_array_init_t>();
    decl->expr = init;

    do {
      // pop new lines
      while (stream_.found(TOK_EOL));
      // pop a value
      const token_t *num = stream_.pop();
      switch (num->type_) {
      case TOK_INT:
      case TOK_FLOAT:
      case TOK_STRING:
      case TOK_NONE:
        init->item.push_back(num);
        break;
      default:
        nano_.errors().bad_array_init_value(*num);
        break;
      }
    } while (stream_.found(TOK_COMMA));
  }

  return decl;
}

ast_node_t* parser_t::parse_decl_var_(const token_t &t) {
  (void)t;
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //        V
  //    var   <TOK_IDENT> [ = <expr> ]

  const token_t *name = stream_.pop(TOK_IDENT);

  // check for [ <TOK_INT> ] array declaration
  if (const token_t *bracket = stream_.found(TOK_LBRACKET)) {
    (void)bracket;
    // pass control to a specialized array decl parser
    return parse_decl_array_(*name);
  }

  auto *decl = ast.alloc<ast_decl_var_t>(name, ast_decl_var_t::e_local);

  // parse assignment expression
  if (const token_t *assign = stream_.found(TOK_ASSIGN)) {
    (void)assign;
    ast_node_t *expr = parse_expr_();
    decl->expr = expr;
  }

  return decl;
}

ast_node_t* parser_t::parse_assign_(const token_t &name) {
  auto &ast = nano_.ast();

  // format:
  //                  V
  //    <TOK_IDENT> =   <expr>

  auto *stmt = ast.alloc<ast_stmt_assign_var_t>(&name);
  stmt->expr = parse_expr_();

  return stmt;
}

ast_exp_call_t* parser_t::parse_call_(const token_t &t) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //                  V
  //    <expr> (   <expr> [ , <expr> ]* )

  auto *expr = ast.alloc<ast_exp_call_t>(&t);

  // parse argument
  if (!stream_.found(TOK_RPAREN)) {
    do {
      auto *arg = parse_expr_();
      expr->args.push_back(arg);
    } while (stream_.found(TOK_COMMA));
    stream_.pop(TOK_RPAREN);
  }

  return expr;
}

ast_node_t* parser_t::parse_if_(const token_t &t) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //       V
  //    if   ( <expr> ) '\n'
  //      <statements>
  //  [ else '\n'
  //      <statements> ]
  //    end '\n'

  auto *stmt = ast.alloc<ast_stmt_if_t>(&t);

  // IF condition
  stream_.pop(TOK_LPAREN);
  stmt->expr = parse_expr_();
  stream_.pop(TOK_RPAREN);
  stream_.pop(TOK_EOL);

  // IF body
  stmt->then_block = ast.alloc<ast_block_t>();
  bool has_else = false;
  while (!stream_.found(TOK_END)) {
    if (stream_.found(TOK_ELSE)) {
      stream_.pop(TOK_EOL);
      has_else = true;
      break;
    }
    ast_node_t *s = parse_stmt_();
    stmt->then_block->add(s);
  }
  // ELSE body
  if (has_else) {
    stmt->else_block = ast.alloc<ast_block_t>();
    while (!stream_.found(TOK_END)) {
      ast_node_t *s = parse_stmt_();
      stmt->else_block->add(s);
    }
  }

  // note: no need to pop newline as parse_stmt() handles that
  return stmt;
}

ast_node_t* parser_t::parse_while_(const token_t &t) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //          V
  //    while   ( <expr> ) '\n'
  //      <statments>
  //    end '\n'

  auto *stmt = ast.alloc<ast_stmt_while_t>(&t);

  // WHILE condition
  stream_.pop(TOK_LPAREN);
  stmt->expr = parse_expr_();
  stream_.pop(TOK_RPAREN);
  stream_.pop(TOK_EOL);

  // WHILE body
  stmt->body = ast.alloc<ast_block_t>();
  while (!stream_.found(TOK_END)) {
    ast_node_t *child = parse_stmt_();
    stmt->body->add(child);
  }

  // note: no need to pop newline as parse_stmt() handles that

  return stmt;
}

ast_node_t* parser_t::parse_for_(const token_t &t) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //        V
  //    for   ( <TOK_IDENT> = <expr> to <expr> ) '\n'
  //      <statments>
  //    end '\n'

  auto *stmt = ast.alloc<ast_stmt_for_t>(&t);

  // FOR initalizer
  stream_.pop(TOK_LPAREN);
  stmt->name = stream_.pop(TOK_IDENT);
  stream_.pop(TOK_ASSIGN);
  stmt->start = parse_expr_();
  stream_.pop(TOK_TO);
  stmt->end = parse_expr_();
  stream_.pop(TOK_RPAREN);
  stream_.pop(TOK_EOL);

  // FOR body
  stmt->body = ast.alloc<ast_block_t>();
  while (!stream_.found(TOK_END)) {
    ast_node_t *child = parse_stmt_();
    stmt->body->add(child);
  }

  // note: no need to pop newline as parse_stmt() handles that
  return stmt;
}

ast_node_t* parser_t::parse_return_(const token_t &t) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //           V
  //    return   [ <expr> ]

  auto node = ast.alloc<ast_stmt_return_t>(&t);

  if (stream_.peek()->type_ == TOK_EOL) {
    node->expr = nullptr;
  } else {
    node->expr = parse_expr_();
  }

  return node;
}

// format:
//    <TOK_IDENT> + = <expr>
//    <TOK_IDENT> - = <expr>
//    <TOK_IDENT> * = <expr>
//    <TOK_IDENT> / = <expr>
ast_node_t* parser_t::parse_compound_(const token_t &t) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  const token_t *op = stream_.pop();
  if (!stream_.found(TOK_ASSIGN)) {
    nano_.errors().equals_expected_after_operator(t);
  }

  auto *assign = ast.alloc<ast_stmt_assign_var_t>(&t);
  auto *bin_op = ast.alloc<ast_exp_bin_op_t>(op);

  bin_op->op = op->type_;
  bin_op->left = ast.alloc<ast_exp_ident_t>(&t);
  bin_op->right = parse_expr_();
  assign->expr = bin_op;

  return assign;
}

ast_node_t* parser_t::parse_stmt_() {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //    [ '\n' ]+
  //    var <TOK_IDENT> [ = <expr> ] '\n'
  //    str <TOK_IDENT> [ = <expr> ] '\n'
  //    <TOK_IDENT> ( <expression list> ) '\n'
  //    <TOK_IDENT> = <expr> '\n'
  //    <TOK_IDENT> <op> = <expr> '\n'
  //    if ( <expr> ) '\n'
  //    while ( <expr> ) '\n'
  //    for ( <TOK_IDENT> = <expr> to <expr> )
  //    return <expr> '\n'

  // consume any blank lines
  while (stream_.found(TOK_EOL));

  ast_node_t *stmt = nullptr;
  const token_t *t = stream_.pop();

  switch (t->type_) {
  case TOK_VAR:
    // var ...
    stmt = parse_decl_var_(*t);
    break;
  case TOK_IDENT:
    switch (stream_.peek()->type_) {
    case TOK_ADD:
    case TOK_SUB:
    case TOK_MUL:
    case TOK_DIV:
      stmt = parse_compound_(*t);
      break;
    case TOK_ASSIGN:
      // x = ...
      stream_.pop();
      stmt = parse_assign_(*t);
      break;
    case TOK_LPAREN:
    {
      // x( 
      const token_t *c = stream_.pop();
      ast_node_t *expr = parse_call_(*c);
      ast_exp_call_t *call = expr->cast<ast_exp_call_t>();
      ast_node_t *callee = ast.alloc<ast_exp_ident_t>(t);
      call->callee = callee;
      stmt = ast.alloc<ast_stmt_call_t>(call);
      break;
    }
    case TOK_LBRACKET:
      // x[
      stream_.pop();
      stmt = parse_array_set_(*t);
      break;
    default:
      nano_.errors().assign_or_call_expected_after(*t);
    }
    break;
  case TOK_IF:
    stmt = parse_if_(*t);
    break;
  case TOK_WHILE:
    stmt = parse_while_(*t);
    break;
  case TOK_FOR:
    stmt = parse_for_(*t);
    break;
  case TOK_RETURN:
    stmt = parse_return_(*t);
    break;
  default:
    nano_.errors().statement_expected(*t);
  }

  // all statements should be on their own line so must have one or more EOLs
  stream_.pop(TOK_EOL);
  // pop any extra EOLS now so we can sync with TOK_END correctly
  while (stream_.found(TOK_EOL));

  return stmt;
}

ast_node_t* parser_t::parse_function_(const token_t &t) {
  (void)t;
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //             V
  //    function   <TOK_IDENT> ( [ <TOK_IDENT> [ , <TOK_IDENT> ]+ ] )
  //      <statements>
  //    end

  // parse function decl.
  const token_t *name = stream_.pop(TOK_IDENT);
  assert(name);

  auto *func = ast.alloc<ast_decl_func_t>(name);
  assert(func);

  // argument list
  stream_.pop(TOK_LPAREN);
  if (!stream_.found(TOK_RPAREN)) {
    do {

      const token_t *arg = stream_.pop(TOK_IDENT);
      auto *a = ast.alloc<ast_decl_var_t>(arg, ast_decl_var_t::e_arg);
      func->args.push_back(a);

    } while (stream_.found(TOK_COMMA));
    stream_.pop(TOK_RPAREN);
  }
  stream_.pop(TOK_EOL);

  // function body
  func->body = ast.alloc<ast_block_t>();
  assert(func->body);

  while (!stream_.found(TOK_END)) {
    ast_node_t *stmt = parse_stmt_();
    func->body->add(stmt);
  }

  return func;
}

ast_node_t* parser_t::parse_array_get_(const token_t &name) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //                  V
  //    <TOK_IDENT> [   <expr> ]

  auto *expr = ast.alloc<ast_exp_array_t>(&name);
  expr->index = parse_expr_();

  // expect a closing bracket
  stream_.pop(TOK_RBRACKET);

  return expr;
}

ast_node_t* parser_t::parse_array_set_(const token_t &name) {
  token_stream_t &stream_ = nano_.lexer().stream();
  auto &ast = nano_.ast();

  // format:
  //                  V
  //    <TOK_IDENT> [   <expr> ] = <expr>

  auto *stmt = ast.alloc<ast_stmt_assign_array_t>(&name);

  // parse the subscript expression
  stmt->index = parse_expr_();

  stream_.pop(TOK_RBRACKET);
  stream_.pop(TOK_ASSIGN);

  // the expression to assign
  stmt->expr = parse_expr_();

  return stmt;
}

ast_node_t* parser_t::parse_const_(const token_t &var) {

  // format:
  //          V
  //    const   <TOK_IDENT> = <TOK_INT>

  ast_node_t *decl = parse_decl_var_(var);
  if (auto *d = decl->cast<ast_decl_var_t>()) {
    d->is_const = true;
    d->scope = ast_decl_var_t::e_global;
  }
  return decl;
}

ast_node_t* parser_t::parse_global_(const token_t &var) {

  // format:
  //        V
  //    var   <TOK_IDENT> = <TOK_INT>
  //    var   <TOK_IDENT> [ <TOK_INT> ]

  ast_node_t *decl = parse_decl_var_(var);
  if (auto *d = decl->cast<ast_decl_var_t>()) {
    d->is_const = false;
    d->scope = ast_decl_var_t::e_global;
  }
  return decl;
}

void parser_t::op_reduce_() {
  auto &ast = nano_.ast();

  // pop operator
  assert(!op_stack_.empty());
  const token_t *op = op_stack_.back();
  op_stack_.pop_back();

  // if this is a binary operator
  if (op->is_binary_op()) {
    assert(exp_stack_.size() >= 2);
    auto *expr = ast.alloc<ast_exp_bin_op_t>(op);
    expr->left = exp_stack_.rbegin()[1];
    expr->right = exp_stack_.rbegin()[0];
    exp_stack_.pop_back();
    exp_stack_.back() = expr;

  // if this is a unary operator
  } else if (op->is_unary_op()) {
    assert(exp_stack_.size() >= 1);
    auto *expr = ast.alloc<ast_exp_unary_op_t>(op);
    expr->child = exp_stack_.back();
    exp_stack_.back() = expr;
  }
}

void parser_t::op_push_(const token_t *op, uint32_t tide) {
  // walk the operator stack for the current expression
  while (op_stack_.size() > tide) {
    // get the top operator on the stack
    const token_t *top = op_stack_.back();
    // if this operator is higher precedence then top of stack
    if (op_type_(op->type_) > op_type_(top->type_)) {
      break;
    }
    // if lower or = precedence, pop from the stack and emit it
    op_reduce_();
  }
  // push this token on the top of the stack
  op_stack_.push_back(op);
}

void parser_t::op_pop_all_(uint32_t tide) {
  // walk the operator stack for the current expression
  while (op_stack_.size() > tide) {
    // emit this operator
    op_reduce_();
  }
}

void parser_t::reset() {
  op_stack_.clear();
}

parser_t::parser_t(nano_t &c)
  : nano_(c)
{
}
