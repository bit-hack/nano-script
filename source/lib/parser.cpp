#include "parser.h"
#include "assembler.h"
#include "lexer.h"
#include "token.h"
#include "errors.h"


using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
bool parser_t::parse(error_t &error) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    var <TOK_IDENT> [ = <TOK_VAL> ]
  //    function <TOK_IDENT> ( [ <TOK_IDENT> [ , <TOK_IDENT> ]+ ] )

  try {
    // enter global scope
    scope_.enter();

    while (!stream_.found(TOK_EOF)) {
      if (stream_.found(TOK_EOL)) {
        // consume any blank lines
        continue;
      }
      if (stream_.found(TOK_VAR)) {
        parse_global_();
        continue;
      }
      if (stream_.found(TOK_FUNC)) {
        parse_function_();
        continue;
      }

      const token_t *tok = stream_.pop();
      assert(tok);
      ccml_.errors().unexpected_token(*tok);
    }

    scope_.leave();
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

void parser_t::ident_load_(const token_t &t) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // try to load from local variable
  if (const identifier_t *ident = scope_.find_ident(t)) {
    if (ident->is_array()) {
      // array load requires subscript operator
      ccml_.errors().ident_is_array_not_var(t);
    }
    if (ident->is_global) {
      asm_.emit(INS_GETG, ident->offset, &t);
      return;
    }
    { /* local variable */
      asm_.emit(INS_GETV, ident->offset, &t);
      return;
    }
  }
  // unable to find identifier
  ccml_.errors().unknown_identifier(t);
}

void parser_t::ident_save_(const token_t &t) {
  assembler_t &asm_ = ccml_.assembler();

  // assign to local variable
  if (const identifier_t *ident = scope_.find_ident(t)) {
    if (ident->is_array()) {
      // array store requires subscript operator
      ccml_.errors().ident_is_array_not_var(t);
    }
    if (ident->is_global) {
      asm_.emit(INS_SETG, ident->offset, &t);
      return;
    }
    { /* load variable */
      asm_.emit(INS_SETV, ident->offset, &t);
      return;
    }
  }
  // cant locate variable
  ccml_.errors().cant_assign_unknown_var(t);
}

void parser_t::parse_lhs_() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    [-] ( <expr> )
  //    [-] <TOK_IDENT>
  //    [-] <TOK_IDENT> ( ... )
  //    [-] <TOK_IDENT> [ ... ]
  //    [-] <TOK_VAL>

  const token_t *neg = stream_.found(TOK_SUB);
  do {

    // ( <expr> )
    if (stream_.found(TOK_LPAREN)) {
      parse_expr_();
      stream_.found(TOK_RPAREN);
      break;
    }

    if (const token_t *t = stream_.found(TOK_IDENT)) {

      // <TOK_IDENT> ( ... )
      if (stream_.found(TOK_LPAREN)) {
        // call function
        parse_call_(*t);

      // <TOK_IDENT> [ ... ]
      } else if (stream_.found(TOK_LBRACKET)) {
        // array access
        parse_array_get_(*t);

      // <TOK_IDENT>
      } else {
        // load a local/global
        ident_load_(*t);
      }
      break;
    }

    // <TOK_VAL>
    if (const token_t *t = stream_.found(TOK_VAL)) {
      asm_.emit(INS_CONST, t->val_, t);
      break;
    }

    ccml_.errors().expecting_lit_or_ident(*stream_.pop());

  } while (false);

  // insert unary minus operator
  if (neg) {
    asm_.emit(INS_NEG, neg);
  }
}

void parser_t::parse_expr_ex_(uint32_t tide) {
  token_stream_t &stream_ = ccml_.lexer().stream_;
  assembler_t &asm_ = ccml_.assembler();

  // format:
  //    <lhs>
  //    <lhs> <op> <expr_ex>
  //    'not' <lhs> <op> <expr_ex>
  //    'not' <lhs>

  if (const token_t *not = stream_.found(TOK_NOT)) {
    parse_expr_ex_(tide);
    op_push_(TOK_NOT, tide);
  }
  else {
    parse_lhs_();
    if (is_operator_()) {
      const token_t *op = stream_.pop();
      op_push_(op->type_, tide);
      parse_expr_ex_(tide);
    }
  }
}

void parser_t::parse_expr_() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //    <expr_ex>

  const uint32_t tide = op_stack_.size();
  parse_expr_ex_(tide);
  op_pop_all_(tide);
}

void parser_t::parse_decl_array_(const token_t &name) {
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

void parser_t::parse_decl_() {
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
    parse_decl_array_(*name);
    return;
  }

  // parse assignment expression
  const token_t *assign = stream_.found(TOK_ASSIGN);
  if (assign) {
    parse_expr_();
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

void parser_t::parse_assign_(const token_t &name) {
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
  parse_expr_();
  // generate assignment instruction
  ident_save_(name);
}

void parser_t::parse_accumulate_(const token_t &name) {
  assembler_t &asm_ = ccml_.assembler();

  // format:
  //                   V
  //    <TOK_IDENT> +=   <expr>

  // check its not an array type
  const identifier_t *ident = scope_.find_ident(name);
  if (ident && ident->is_array()) {
    ccml_.errors().assign_to_array_missing_bracket(name);
  }

  // parse increment expression
  parse_expr_();

  if (ident->is_array() || ident->is_global) {
    // load, accumulate, save
    ident_load_(name);
    asm_.emit(INS_ADD, &name);
    ident_save_(name);
  }
  else {
    asm_.emit(INS_ACCV, ident->offset, &name);
  }
}

void parser_t::parse_call_(const token_t &name) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //                  V
  //    <TOK_IDENT> (   <expr> [ , <expr> ]* )

  // parse argument
  int32_t arg_count = 0;
  if (!stream_.found(TOK_RPAREN)) {
    do {
      parse_expr_();
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

void parser_t::parse_if_() {
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
  parse_expr_();

  // this jump skips the body of the if, hence not
  asm_.emit(INS_CJMP, 0);
  int32_t *l1 = asm_.get_fixup();

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
      parse_stmt_();
    }
    scope_.leave();
  }

  int32_t *l2 = nullptr;
  if (has_else) {
    // skip over ELSE body
    asm_.emit(INS_JMP, 0);
    l2 = asm_.get_fixup();
  }
  *l1 = asm_.pos();
  if (has_else) {
    // ELSE body
    {
      scope_.enter();
      while (!stream_.found(TOK_END)) {
        parse_stmt_();
      }
      scope_.leave();
    }
    // END
    *l2 = asm_.pos();
  }

  // note: no need to pop newline as parse_stmt() handles that
}

void parser_t::parse_while_() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //          V
  //    while   ( <expr> ) '\n'
  //      <statments>
  //    end '\n'

  // XXX: consider moving the expression to the bottom of the loop, and having
  //      one initial jump to the end of the loop.  this way we can save one
  //      INS_JMP every iteration of the loop.

  // top of loop
  const int32_t l1 = asm_.pos();
  // WHILE condition
  stream_.pop(TOK_LPAREN);
  parse_expr_();

  // GOTO end if false
  asm_.emit(INS_CJMP, 0);
  int32_t *l2 = asm_.get_fixup();

  // note: moved below not and jump to keep line table correct
  stream_.pop(TOK_RPAREN);
  stream_.pop(TOK_EOL);

  // WHILE body
  {
    scope_.enter();
    while (!stream_.found(TOK_END)) {
      parse_stmt_();
    }
    scope_.leave();
  }
  // note: no need to pop newline as parse_stmt() handles that

  // unconditional jump to top
  asm_.emit(INS_JMP, l1);
  // WHILE end
  *l2 = asm_.pos();
}

void parser_t::parse_return_() {
  assembler_t &asm_ = ccml_.assembler();

  // format:
  //           V
  //    return   <expr>

  // TODO: we need to fixup all of the return sizes after the function has been
  //       processed in total so that it matches the INS_LOCALS size

  parse_expr_();
  asm_.emit(INS_RET, scope_.arg_count() + scope_.var_count());
  int32_t *fixup = asm_.get_fixup();

  // TODO: add fixup to a list here for process at function end
  funcs_.back().add_return_fixup(fixup);
}

void parser_t::parse_stmt_() {
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
    parse_decl_();
  } else if (const token_t *var = stream_.found(TOK_IDENT)) {
    if (stream_.found(TOK_ASSIGN)) {
      // x = ...
      parse_assign_(*var);
    } else if (stream_.found(TOK_ACC)) {
      // x += ...
      parse_accumulate_(*var);
    } else if (stream_.found(TOK_LPAREN)) {
      // x(
      parse_call_(*var);
      // note: we throw away the return value since its not being used
      asm_.emit(INS_POP, 1);
    } else if (stream_.found(TOK_LBRACKET)) {
      // x[
      parse_array_set_(*var);
    } else {
      ccml_.errors().assign_or_call_expected_after(*var);
    }
  } else if (stream_.found(TOK_IF)) {
    parse_if_();
  } else if (stream_.found(TOK_WHILE)) {
    parse_while_();
  } else if (stream_.found(TOK_RETURN)) {
    parse_return_();
  } else {
    ccml_.errors().statement_expected();
  }

  // all statements should be on their own line
  stream_.pop(TOK_EOL);
}

void parser_t::parse_function_() {
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
  int32_t *locals = asm_.get_fixup();

  // note: at this point we parse all of the statements inside
  //       the function

  // function body
  {
    scope_.enter();
    while (!stream_.found(TOK_END)) {
      parse_stmt_();
    }
    scope_.leave();
  }

  // fixup the number of locals we are reserving with INS_LOCALS
  const uint32_t num_locals = scope_.max_depth();
  if (num_locals) {
    *locals = num_locals;
  }

  const int32_t ret_size = scope_.arg_count() + num_locals;

  // emit dummy epilogue (may be unreachable)
  asm_.emit(INS_CONST, 0);
  asm_.emit(INS_RET, ret_size);

  // fix all of the return operands to match INS_LOCALS size
  funcs_.back().fix_return_operands(ret_size);

  // return the max scope depth
  scope_.leave_function();

  // flush any pending instructions
  asm_.flush();
}

void parser_t::parse_array_get_(const token_t &name) {
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
    parse_expr_();

    // if array is global we are relative to bottom of stack
    if (ident->is_global) {
      asm_.emit(INS_GETGI, ident->offset, &name);
    }
    // if array is local we are frame relative
    else {
      asm_.emit(INS_GETVI, ident->offset, &name);
    }
    // expect a closing bracket
    stream_.pop(TOK_RBRACKET);
  }
  else {
    ccml_.errors().use_of_unknown_array(name);
  }
}

void parser_t::parse_array_set_(const token_t &name) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //                  V
  //    <TOK_IDENT> [   <expr> ] = <expr>

  // parse the subscript expression
  parse_expr_();

  stream_.pop(TOK_RBRACKET);
  stream_.pop(TOK_ASSIGN);

  // the expression to assign
  parse_expr_();

  if (const identifier_t *ident = scope_.find_ident(name)) {

    // check that this was an array type
    if (!ident->is_array()) {
      ccml_.errors().variable_is_not_array(name);
    }

    // if array is global we are relative to bottom of stack
    if (ident->is_global) {
      asm_.emit(INS_SETGI, ident->offset, &name);
    }
    // if array is local we are frame relative
    else {
      asm_.emit(INS_SETVI, ident->offset, &name);
    }
  }
  else {
    ccml_.errors().assign_to_unknown_array(name);
  }
}

void parser_t::parse_global_() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // format:
  //        V
  //    var   <TOK_IDENT> = <TOK_VAL>
  //    var   <TOK_IDENT> [ <TOK_VAL> ]

  const token_t *name = stream_.pop(TOK_IDENT);

  // check for duplicate globals
  for (const auto &g : global_) {
    if (g.token_->str_ == name->str_) {
      ccml_.errors().global_already_exists(*name);
    }
  }

  // parse global array decl
  if (stream_.found(TOK_LBRACKET)) {
    const token_t *size = stream_.pop(TOK_VAL);
    stream_.pop(TOK_RBRACKET);
    // validate array size
    if (size->val_ <= 1) {
      ccml_.errors().array_size_must_be_greater_than(*name);
    }
    scope_.var_add(*name, size->val_);
    global_t global = {name, 0, 0, size->val_};
    // add to global list
    global_.push_back(global);
  }
  // parse generic global variable
  else {
    // add to the globals list
    scope_.var_add(*name, 1);
    global_t global = {name, 0, 0, 1};
    // assign a default value
    if (stream_.found(TOK_ASSIGN)) {
      const token_t *value = stream_.pop(TOK_VAL);
      global.value_ = value->val_;
    }
    // add to global list
    global_.push_back(global);
  }
}

void parser_t::op_push_(token_e op, uint32_t tide) {
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // walk the operator stack for the current expression
  while (op_stack_.size() > tide) {
    // get the top operator on the stack
    const token_e top = op_stack_.back();
    // if this operator is higher precedence then top of stack
    if (op_type_(op) > op_type_(top)) {
      break;
    }
    // if lower or = precedence, pop from the stack and emit it
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
