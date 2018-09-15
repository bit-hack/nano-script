#include "parser.h"
#include "assembler.h"
#include "lexer.h"
#include "token.h"
#include "errors.h"
#include "ast.h"
#include "instructions.h"


using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
bool parser_t::parse(error_t &error) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  ast_program_t *program = new ast_program_t;

  // format:
  //    var <TOK_IDENT> [ = <TOK_VAL> ]
  //    function <TOK_IDENT> ( [ <TOK_IDENT> [ , <TOK_IDENT> ]+ ] )

  try {

    const token_t *t = nullptr;

    while (!stream_.found(TOK_EOF)) {
      if (stream_.found(TOK_EOL)) {
        // consume any blank lines
        continue;
      }
      if (t = stream_.found(TOK_VAR)) {
        if (ast_node_t *node = parse_global_()) {
          program->children.push_back(node);
        }
        continue;
      }
      if (t = stream_.found(TOK_FUNC)) {
        if (ast_node_t *func = parse_function_(*t)) {
          program->children.push_back(func);
        }
        continue;
      }

      const token_t *tok = stream_.pop();
      assert(tok);
      ccml_.errors().unexpected_token(*tok);
    }
  }
  catch (const error_t &e) {
    error = e;
    return false;
  }

  return true;
}

const function_t *parser_t::find_function(const token_t *name, bool can_fail) const {
  token_stream_t &stream_ = ccml_.lexer().stream_;
  for (const function_t &func : funcs_) {
    if (func.name_ == name->str_) {
      return &func;
    }
  }
  if (!can_fail) {
    ccml_.errors().unknown_function(*name);
  }
  return nullptr;
}

const function_t *parser_t::find_function(const std::string &name) const {
  token_stream_t &stream_ = ccml_.lexer().stream_;
  for (const function_t &func : funcs_) {
    if (func.name_ == name) {
      return &func;
    }
  }
  return nullptr;
}

const function_t *parser_t::find_function(uint32_t id) const {
  if (funcs_.size() > id) {
    return &funcs_[id];
  }
  return nullptr;
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
  token_stream_t &stream_ = ccml_.lexer().stream_;
  return op_type_(stream_.type()) > 0;
}

void parser_t::parse_lhs_() {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    [-] ( <expr> )
  //    [-] <TOK_IDENT>
  //    [-] <TOK_IDENT> ( ... )
  //    [-] <TOK_IDENT> [ ... ]
  //    [-] <TOK_VAL>

//  const token_t *neg = stream_.found(TOK_SUB);
  do {

    // ( <expr> )
    if (stream_.found(TOK_LPAREN)) {
      ast_node_t *expr = parse_expr_();
      exp_stack_.push_back(expr);
      stream_.found(TOK_RPAREN);
      break;
    }

    if (const token_t *t = stream_.found(TOK_IDENT)) {

      // <TOK_IDENT> ( ... )
      if (stream_.found(TOK_LPAREN)) {
        // call function
        ast_node_t *expr = parse_call_(*t);
        exp_stack_.push_back(expr);
        break;

      // <TOK_IDENT> [ ... ]
      } else if (stream_.found(TOK_LBRACKET)) {
        // array access
        ast_exp_array_t *expr = new ast_exp_array_t(t);
        expr->index = parse_expr_();
        exp_stack_.push_back(expr);
        // expect a closing bracket
        stream_.pop(TOK_RBRACKET);
        break;

      // <TOK_IDENT>
      } else {
        // load a local/global
        ast_node_t *expr = new ast_exp_ident_t(t);
        exp_stack_.push_back(expr);
        break;
      }
    }

    // <TOK_VAL>
    if (const token_t *t = stream_.found(TOK_VAL)) {
      ast_node_t *expr = new ast_exp_const_t(t);
      exp_stack_.push_back(expr);
      break;
    }

    ccml_.errors().expecting_lit_or_ident(*stream_.pop());

  } while (false);

  // insert unary minus operator
//  if (neg) {
//    asm_.emit(INS_NEG, neg);
//  }
}

void parser_t::parse_expr_ex_(uint32_t tide) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    <lhs>
  //    <lhs> <op> <expr_ex>
  //    'not' <lhs> <op> <expr_ex>
  //    'not' <lhs>

  if (const token_t *not = stream_.found(TOK_NOT)) {
    parse_expr_ex_(tide);
    op_push_(not, tide);
  }
  else {
    parse_lhs_();
    if (is_operator_()) {
      const token_t *op = stream_.pop();
      op_push_(op, tide);
      parse_expr_ex_(tide);
    }
  }
}

ast_node_t* parser_t::parse_expr_() {
  token_stream_t &stream_ = ccml_.lexer().stream_;

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
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // TODO: add array initialization

  // format:
  //                        V
  //    var <TOK_IDENT> '['   <TOK_VAR> ']'

  const token_t *size = stream_.pop(TOK_VAL);
  auto *decl = new ast_decl_array_t{&name};
  decl->size = size;
  stream_.pop(TOK_RBRACKET);

  return decl;
}

ast_node_t* parser_t::parse_decl_(const token_t &t) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //        V
  //    var   <TOK_IDENT> [ = <expr> ]

  const token_t *name = stream_.pop(TOK_IDENT);

  // check for [ <TOK_VAL> ] array declaration
  if (const token_t *bracket = stream_.found(TOK_LBRACKET)) {
    // pass control to a specialized array decl parser
    return parse_decl_array_(*name);
  }

  auto *decl = new ast_decl_var_t(name);

  // parse assignment expression
  if (const token_t *assign = stream_.found(TOK_ASSIGN)) {
    ast_node_t *expr = parse_expr_();
    decl->expr = expr;
  }

  return decl;
}

ast_node_t* parser_t::parse_assign_(const token_t &name) {

  // format:
  //                  V
  //    <TOK_IDENT> =   <expr>

  auto *stmt = new ast_stmt_assign_var_t(&name);
  stmt->expr = parse_expr_();

  return stmt;
}

ast_node_t* parser_t::parse_call_(const token_t &name) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //                  V
  //    <TOK_IDENT> (   <expr> [ , <expr> ]* )

  auto *expr = new ast_exp_call_t(&name);

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
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //       V
  //    if   ( <expr> ) '\n'
  //      <statements>
  //  [ else '\n'
  //      <statements> ]
  //    end '\n'

  auto *stmt = new ast_stmt_if_t(&t);

  // IF condition
  stream_.pop(TOK_LPAREN);
  stmt->expr = parse_expr_();
  stream_.pop(TOK_RPAREN);
  stream_.pop(TOK_EOL);

  // IF body
  bool has_else = false;
  while (!stream_.found(TOK_END)) {
    if (stream_.found(TOK_ELSE)) {
      stream_.pop(TOK_EOL);
      has_else = true;
      break;
    }
    ast_node_t *s = parse_stmt_();
    stmt->then_block.push_back(s);
  }
  // ELSE body
  if (has_else) {
    while (!stream_.found(TOK_END)) {
      ast_node_t *s = parse_stmt_();
      stmt->else_block.push_back(s);
    }
  }

  // note: no need to pop newline as parse_stmt() handles that
  return stmt;
}

ast_node_t* parser_t::parse_while_(const token_t &t) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //          V
  //    while   ( <expr> ) '\n'
  //      <statments>
  //    end '\n'

  auto *stmt = new ast_stmt_while_t(&t);

  // WHILE condition
  stream_.pop(TOK_LPAREN);
  stmt->expr = parse_expr_();
  stream_.pop(TOK_RPAREN);
  stream_.pop(TOK_EOL);

  // WHILE body
  while (!stream_.found(TOK_END)) {
    ast_node_t *child = parse_stmt_();
    stmt->body.push_back(child);
  }

  // note: no need to pop newline as parse_stmt() handles that

  return stmt;
}

ast_node_t* parser_t::parse_return_(const token_t &t) {
  assembler_t &asm_ = ccml_.assembler();

  // format:
  //           V
  //    return   <expr>

  auto node = new ast_stmt_return_t(&t);
  node->expr = parse_expr_();

  return node;
}

ast_node_t* parser_t::parse_stmt_() {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    [ '\n' ]+
  //    var <TOK_IDENT> [ = <expr> ] '\n'
  //    <TOK_IDENT> ( <expression list> ) '\n'
  //    if ( <expr> ) '\n'
  //    while ( <expr> ) '\n'
  //    return <expr> '\n'

  while (stream_.found(TOK_EOL)) {
    // consume any blank lines
  }

  const token_t *t = nullptr;
  ast_node_t *stmt = nullptr;

  if (t = stream_.found(TOK_VAR)) {
    // var ...
    stmt = parse_decl_(*t);
  } else if (const token_t *var = stream_.found(TOK_IDENT)) {
    if (stream_.found(TOK_ASSIGN)) {
      // x = ...
      stmt = parse_assign_(*var);
    } else if (stream_.found(TOK_LPAREN)) {
      // x(
      stmt = parse_call_(*var);
    } else if (stream_.found(TOK_LBRACKET)) {
      // x[
      stmt = parse_array_set_(*var);
    } else {
      ccml_.errors().assign_or_call_expected_after(*var);
    }
  } else if (t = stream_.found(TOK_IF)) {
    stmt = parse_if_(*t);
  } else if (t = stream_.found(TOK_WHILE)) {
    stmt = parse_while_(*t);
  } else if (t = stream_.found(TOK_RETURN)) {
    stmt = parse_return_(*t);
  } else {
    ccml_.errors().statement_expected();
  }

  // all statements should be on their own line
  stream_.pop(TOK_EOL);
  return stmt;
}

ast_node_t* parser_t::parse_function_(const token_t &t) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //             V
  //    function   <TOK_IDENT> ( [ <TOK_IDENT> [ , <TOK_IDENT> ]+ ] )
  //      <statements>
  //    end

  // parse function decl.
  const token_t *name = stream_.pop(TOK_IDENT);
  assert(name);

  auto *func = new ast_decl_func_t(name);

  // argument list
  stream_.pop(TOK_LPAREN);
  if (!stream_.found(TOK_RPAREN)) {
    do {
      const token_t *arg = stream_.pop(TOK_IDENT);
      func->args.push_back(arg);
    } while (stream_.found(TOK_COMMA));
    stream_.pop(TOK_RPAREN);
  }
  stream_.pop(TOK_EOL);

  // function body
  scope_.enter();
  while (!stream_.found(TOK_END)) {
    ast_node_t *stmt = parse_stmt_();
    func->body.push_back(stmt);
  }
  scope_.leave();

  return func;
}

ast_node_t* parser_t::parse_array_get_(const token_t &name) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //                  V
  //    <TOK_IDENT> [   <expr> ]

  auto *expr = new ast_exp_array_t(&name);
  expr->index = parse_expr_();

  // expect a closing bracket
  stream_.pop(TOK_RBRACKET);

  return expr;
}

ast_node_t* parser_t::parse_array_set_(const token_t &name) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //                  V
  //    <TOK_IDENT> [   <expr> ] = <expr>

  auto *stmt = new ast_stmt_assign_array_t(&name);

  // parse the subscript expression
  stmt->index = parse_expr_();

  stream_.pop(TOK_RBRACKET);
  stream_.pop(TOK_ASSIGN);

  // the expression to assign
  stmt->expr = parse_expr_();

  return stmt;
}

ast_node_t* parser_t::parse_global_() {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //        V
  //    var   <TOK_IDENT> = <TOK_VAL>
  //    var   <TOK_IDENT> [ <TOK_VAL> ]

  const token_t *name = stream_.pop(TOK_IDENT);

  // parse global array decl
  if (stream_.found(TOK_LBRACKET)) {
    ast_decl_array_t *decl = new ast_decl_array_t(name);
    const token_t *size = stream_.pop(TOK_VAL);
    stream_.pop(TOK_RBRACKET);
    // validate array size
    if (size->val_ <= 1) {
      ccml_.errors().array_size_must_be_greater_than(*name);
    }
    decl->size = size;
    return decl;
  }
  // parse global var decl
  else {
    ast_decl_var_t *decl = new ast_decl_var_t(name);
    // assign a default value
    if (stream_.found(TOK_ASSIGN)) {
      const token_t *value = stream_.pop(TOK_VAL);
      decl->expr = new ast_exp_const_t(value);
    }
    return decl;
  }
}

void parser_t::op_push_(const token_t *op, uint32_t tide) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // walk the operator stack for the current expression
  while (op_stack_.size() > tide) {
    // get the top operator on the stack
    const token_t *top = op_stack_.back();
    // if this operator is higher precedence then top of stack
    if (op_type_(op->type_) > op_type_(top->type_)) {
      break;
    }
    // if lower or = precedence, pop from the stack and emit it

    if (ins_is_binary)

    ccml_.assembler().emit(top);
    op_stack_.pop_back();
  }
  // push this token on the top of the stack
  op_stack_.push_back(op);
}

void parser_t::op_pop_all_(uint32_t tide) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // walk the operator stack for the current expression
  while (op_stack_.size() > tide) {
    // emit this operator
    const token_e op = op_stack_.back();
    ccml_.assembler().emit(op);
    op_stack_.pop_back();
  }
}

void parser_t::add_function(const std::string &name, ccml_syscall_t func,
                            uint32_t num_args) {
  funcs_.emplace_back(name, func, num_args, funcs_.size());
}

void parser_t::reset() {
  funcs_.clear();
  global_.clear();
  op_stack_.clear();
}

parser_t::parser_t(ccml_t &c)
  : ccml_(c)
  , scope_(c)
{
  // register builtin functions
  void builtin_abs(struct ccml::thread_t &);
  void builtin_max(struct ccml::thread_t &);
  void builtin_min(struct ccml::thread_t &);
  add_function("abs", builtin_abs, 1);
  add_function("min", builtin_min, 2);
  add_function("max", builtin_max, 2);
}
