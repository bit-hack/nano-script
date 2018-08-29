#include "parser.h"
#include "assembler.h"
#include "lexer.h"
#include "token.h"
#include "errors.h"


bool parser_t::parse() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    var <TOK_IDENT> [ = <TOK_VAL> ]
  //    function <TOK_IDENT> ( [ <TOK_IDENT> [ , <TOK_IDENT> ]+ ] )

  // enter global scope
  scope_.enter();

  while (!stream_.found(TOK_EOF)) {
    if (stream_.found(TOK_EOL)) {
      // consume any blank lines
      continue;
    }
    if (stream_.found(TOK_VAR)) {
      parse_global();
      continue;
    }
    if (stream_.found(TOK_FUNC)) {
      parse_function();
      continue;
    }

    const token_t *tok = stream_.pop();
    assert(tok);
    ccml_.errors().unexpected_token(*tok);
  }

  scope_.leave();

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

int32_t parser_t::op_type(token_e type) const {
  // return precedence
  // note: higher number will be evaluated earlier in an expression
  switch (type) {
  case TOK_AND:
  case TOK_OR:
    return 1;
  case TOK_LT:
  case TOK_GT:
  case TOK_LEQ:
  case TOK_GEQ:
  case TOK_EQ:
    return 2;
  case TOK_ADD:
  case TOK_SUB:
    return 3;
  case TOK_MUL:
  case TOK_DIV:
  case TOK_MOD:
    return 4;
  default:
    // this is not an operator
    return 0;
  }
}

bool parser_t::is_operator() const {
  token_stream_t &stream_ = ccml_.lexer().stream_;
  return op_type(stream_.type()) > 0;
}

void parser_t::ident_load(const token_t &t) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // try to load from local variable
  if (const identifier_t *ident = scope_.find_ident(t)) {
    // check this is not an array type
    if (ident->is_array()) {
      ccml_.errors().ident_is_array_not_var(t);
    }
    else if (ident->is_global) {
      // TODO: check ident->offset and change way GETG works
      asm_.emit(INS_GETG, ident->offset, &t);
      return;
    }
    else {
      asm_.emit(INS_GETV, ident->offset, &t);
      return;
    }
  }
  // try to load from global variable
  for (uint32_t i = 0; i < global_.size(); ++i) {
    if (global_[i].token_->str_ == t.str_) {
      asm_.emit(INS_GETG, i, &t);
      return;
    }
  }
  // unable to find identifier
  ccml_.errors().unknown_identifier(t);
}

void parser_t::ident_save(const token_t &t) {
  assembler_t &asm_ = ccml_.assembler();

  // assign to local variable
  if (const identifier_t *ident = scope_.find_ident(t)) {
    // check this is not an array type
    if (ident->is_array()) {
      ccml_.errors().ident_is_array_not_var(t);
    }
    asm_.emit(INS_SETV, ident->offset, &t);
    return;
  }
  // assign to global variable
  for (uint32_t i = 0; i < global_.size(); ++i) {
    if (global_[i].token_->str_ == t.str_) {
      asm_.emit(INS_SETG, i, &t);
      return;
    }
  }
  // cant locate variable
  ccml_.errors().cant_assign_unknown_var(t);
}

void parser_t::parse_lhs() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    ( <expr> )
  //    <TOK_IDENT>
  //    <TOK_IDENT> ( ... )
  //    <TOK_VAL>

  if (stream_.found(TOK_LPAREN)) {
    parse_expr();
    stream_.found(TOK_RPAREN);
    return;
  }
  if (const token_t *t = stream_.found(TOK_IDENT)) {
    if (stream_.found(TOK_LPAREN)) {
      // call function
      parse_call(*t);
    } else if (stream_.found(TOK_LBRACKET)) {
      // array access
      parse_array_get(*t);
    } else {
      // load a local/global
      ident_load(*t);
    }
    return;
  }
  if (const token_t *t = stream_.found(TOK_VAL)) {
    asm_.emit(INS_CONST, t->val_, t);
    return;
  }
  ccml_.errors().expecting_lit_or_ident(*stream_.pop());
}

void parser_t::parse_expr_ex(uint32_t tide) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    <lhs>
  //    <lhs> <op> <expr_ex>

  parse_lhs();
  if (is_operator()) {
    const token_t *op = stream_.pop();
    op_push(op->type_, tide);
    parse_expr_ex(tide);
  }
}

void parser_t::parse_expr() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    not <expr_ex>
  //    <expr_ex>

  const token_t *not = stream_.found(TOK_NOT);

  uint32_t tide = op_stack_.size();
  parse_expr_ex(tide);
  op_pop_all(tide);

  if (not) {
    asm_.emit(INS_NOT, not);
  }
}

void parser_t::parse_decl_array(const token_t &name) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // TODO: add array initialization

  // format:
  //                        V
  //    var <TOK_IDENT> '['   <TOK_VAR> ']'

  const token_t *size = stream_.pop(TOK_VAL);
  if (size->val_ <= 1) {
    ccml_.errors().array_size_must_be_greater_than(name);
  }

  stream_.pop(TOK_RBRACKET);

  scope_.var_add(name, size->val_);
}

void parser_t::parse_decl() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //        V
  //    var   <TOK_IDENT> [ = <expr> ]

  const token_t *name = stream_.pop(TOK_IDENT);

  // check for duplicate function name
  if (scope_.find_ident(*name)) {
    ccml_.errors().var_already_exists(*name);
  }

  // check for [ <TOK_VAL> ] array declaration
  if (const token_t *bracket = stream_.found(TOK_LBRACKET)) {
    // pass control to a specialize array decl parser
    parse_decl_array(*name);
    return;
  }

  // parse assignment expression
  const token_t *assign = stream_.found(TOK_ASSIGN);
  if (assign) {
    parse_expr();
  } else {
    // implicitly set to zero
    // TODO: add tests for this
    asm_.emit(INS_CONST, 0, name);
  }

  // add name to identifier table
  // note: must happen after parse_expr() to avoid 'var x = x'
  scope_.var_add(*name, 1);

  // generate assignment
  const identifier_t *ident = scope_.find_ident(*name);
  assert(ident && "identifier should always be known here");
  asm_.emit(INS_SETV, ident->offset, assign);
}

void parser_t::parse_assign(const token_t &name) {
  assembler_t &asm_ = ccml_.assembler();

  // format:
  //                  V
  //    <TOK_IDENT> =   <expr>

  // check its not an array type
  const identifier_t *ident = scope_.find_ident(name);
  if (ident && ident->is_array()) {
    ccml_.errors().assign_to_array_missing_bracket(name);
  }

  // parse assignment expression
  parse_expr();
  // generate assignment instruction
  ident_save(name);
}

void parser_t::parse_call(const token_t &name) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //                  V
  //    <TOK_IDENT> (   <expr> [ , <expr> ]* )

  // parse argument
  int32_t arg_count = 0;
  if (!stream_.found(TOK_RPAREN)) {
    do {
      parse_expr();
      ++arg_count;
    } while (stream_.found(TOK_COMMA));
    stream_.pop(TOK_RPAREN);
  }

  const function_t *func = find_function(&name, true);
  if (!func) {
    ccml_.errors().unknown_function(name);
  }
  // check number of arguments
  if (arg_count != func->num_args_) {
    ccml_.errors().wrong_number_of_args(name, func->num_args_, arg_count);
  }

  if (func->sys_) {
    asm_.emit(INS_SCALL, func->id_, &name);
  } else {
    asm_.emit(INS_CALL, func->pos_, &name);
  }
}

void parser_t::parse_if() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //       V
  //    if   ( <expr> ) '\n'
  //      <statements>
  //  [ else '\n'
  //      <statements> ]
  //    end '\n'

  bool has_else = false;
  // IF condition
  stream_.pop(TOK_LPAREN);
  parse_expr();

  // this jump skips the body of the if, hence not
  asm_.emit(INS_NOT);
  int32_t *l1 = asm_.emit(INS_CJMP, 0);

  // note: moved below not and jmp to keep linetable correct
  stream_.pop(TOK_RPAREN);
  stream_.pop(TOK_EOL);

  // IF body
  {
    scope_.enter();
    while (!stream_.found(TOK_END)) {
      if (stream_.found(TOK_ELSE)) {
        stream_.pop(TOK_EOL);
        has_else = true;
        break;
      }
      parse_stmt();
    }
    scope_.leave();
  }

  int32_t *l2 = nullptr;
  if (has_else) {
    // skip over ELSE body
    l2 = asm_.emit(INS_JMP, 0);
  }
  *l1 = asm_.pos();
  if (has_else) {
    // ELSE body
    {
      scope_.enter();
      while (!stream_.found(TOK_END)) {
        parse_stmt();
      }
      scope_.leave();
    }
    // END
    *l2 = asm_.pos();
  }

  // note: no need to pop newline as parse_stmt() handles that
}

void parser_t::parse_while() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //          V
  //    while   ( <expr> ) '\n'
  //      <statments>
  //    end '\n'

  // top of loop
  const int32_t l1 = asm_.pos();
  // WHILE condition
  stream_.pop(TOK_LPAREN);
  parse_expr();

  // GOTO end if false
  asm_.emit(INS_NOT);
  int32_t *l2 = asm_.emit(INS_CJMP, 0);

  // note: moved below not and jump to keep linetable correct
  stream_.pop(TOK_RPAREN);
  stream_.pop(TOK_EOL);

  // WHILE body
  {
    scope_.enter();
    while (!stream_.found(TOK_END)) {
      parse_stmt();
    }
    scope_.leave();
  }
  // note: no need to pop newline as parse_stmt() handles that

  // unconditional jump to top
  asm_.emit(INS_JMP, l1);
  // WHILE end
  *l2 = asm_.pos();
}

void parser_t::parse_return() {
  assembler_t &asm_ = ccml_.assembler();

  // format:
  //           V
  //    return   <expr>

  // TODO: we need to fixup all of the return sizes after the function has been
  //       processed in total so that it matches the INS_LOCALS size

  parse_expr();
  int32_t *fixup = asm_.emit(INS_RET, scope_.arg_count() + scope_.var_count());

  // TODO: add fixup to a list here for process at function end
  funcs_.back().add_return_fixup(fixup);
}

void parser_t::parse_stmt() {
  assembler_t &asm_ = ccml_.assembler();
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

  if (stream_.found(TOK_VAR)) {
    // var ...
    parse_decl();
  } else if (const token_t *var = stream_.found(TOK_IDENT)) {
    if (stream_.found(TOK_ASSIGN)) {
      // x = ...
      parse_assign(*var);
    } else if (stream_.found(TOK_LPAREN)) {
      // x(
      parse_call(*var);
      // note: we throw away the return value since its not being used
      asm_.emit(INS_POP, 1);
    } else if (stream_.found(TOK_LBRACKET)) {
      // x[
      parse_array_set(*var);
    } else {
      ccml_.errors().assign_or_call_expected_after(*var);
    }
  } else if (stream_.found(TOK_IF)) {
    parse_if();
  } else if (stream_.found(TOK_WHILE)) {
    parse_while();
  } else if (stream_.found(TOK_RETURN)) {
    parse_return();
  } else {
    ccml_.errors().statement_expected();
  }

  // all statements should be on their own line
  stream_.pop(TOK_EOL);
}

void parser_t::parse_function() {
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

  if (find_function(name, true)) {
    ccml_.errors().function_already_exists(*name);
  }

  // argument list
  stream_.pop(TOK_LPAREN);
  if (!stream_.found(TOK_RPAREN)) {
    do {
      const token_t *arg = stream_.pop(TOK_IDENT);
      scope_.arg_add(arg);
    } while (stream_.found(TOK_COMMA));
    stream_.pop(TOK_RPAREN);
    scope_.arg_calc_offsets();
  }
  stream_.pop(TOK_EOL);

  // new function container
  funcs_.emplace_back(
    name->str_,
    asm_.pos(),
    scope_.arg_count(),
    funcs_.size());

  // emit dummy prologue
  asm_.emit(INS_LOCALS, 0, name);
  int32_t &locals = asm_.get_fixup();

  // note: at this point we parse all of the statements inside
  //       the function

  // function body
  {
    scope_.enter();
    while (!stream_.found(TOK_END)) {
      parse_stmt();
    }
    scope_.leave();
  }

  // fixup the number of locals we are reserving with INS_LOCALS
  const uint32_t num_locals = scope_.max_depth();
  if (num_locals) {
    locals = num_locals;
  }

  const int32_t ret_size = scope_.arg_count() + num_locals;

  // emit dummy epilogue (may be unreachable)
  asm_.emit(INS_CONST, 0);
  asm_.emit(INS_RET, ret_size);

  // fix all of the return operands to match INS_LOCALS size
  funcs_.back().fix_return_operands(ret_size);

  // return the max scope depth
  scope_.leave_function();
}

void parser_t::parse_array_get(const token_t &name) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //                  V
  //    <TOK_IDENT> [   <expr> ]

  if (const identifier_t *ident = scope_.find_ident(name)) {

    // check that this was an array type
    if (!ident->is_array()) {
      ccml_.errors().variable_is_not_array(name);
    }

    // this expression is the index subscript
    parse_expr();
    // emit an array index
    asm_.emit(INS_GETI, ident->offset, &name);
    // expect a closing bracket
    stream_.pop(TOK_RBRACKET);
  }
  else {
    ccml_.errors().use_of_unknown_array(name);
  }
}

void parser_t::parse_array_set(const token_t &name) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //                  V
  //    <TOK_IDENT> [   <expr> ] = <expr>

  // parse the subscript expression
  parse_expr();

  stream_.pop(TOK_RBRACKET);
  stream_.pop(TOK_ASSIGN);

  // the expression to assign
  parse_expr();

  if (const identifier_t *ident = scope_.find_ident(name)) {

    // check that this was an array type
    if (!ident->is_array()) {
      ccml_.errors().variable_is_not_array(name);
    }

    asm_.emit(INS_SETI, ident->offset, &name);
  }
  else {
    ccml_.errors().assign_to_unknown_array(name);
  }
}

void parser_t::parse_global() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //        V
  //    var   <TOK_IDENT> = <TOK_VAL>

  const token_t *name = stream_.pop(TOK_IDENT);
  global_t global = {name, 0};

  // check for duplicate globals
  for (const auto &g : global_) {
    if (g.token_->str_ == name->str_) {
      ccml_.errors().global_already_exists(*name);
    }
  }

  // add to the globals list
  scope_.var_add(*name, 1);

  if (stream_.found(TOK_ASSIGN)) {
    const token_t *value = stream_.pop(TOK_VAL);
    global.value_ = value->val_;
  }

  global_.push_back(global);
}

void parser_t::op_push(token_e op, uint32_t tide) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // walk the operator stack for the current expression
  while (op_stack_.size() > tide) {
    // get the top operator on the stack
    const token_e top = op_stack_.back();
    // if this operator is higher precedence then top of stack
    if (op_type(op) > op_type(top)) {
      break;
    }
    // if lower or = precedence, pop from the stack and emit it
    ccml_.assembler().emit(top);
    op_stack_.pop_back();
  }
  // push this token on the top of the stack
  op_stack_.push_back(op);
}

void parser_t::op_pop_all(uint32_t tide) {
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
