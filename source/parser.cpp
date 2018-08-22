#include "parser.h"
#include "assembler.h"
#include "lexer.h"
#include "token.h"

bool parser_t::parse() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  while (!stream_.found(TOK_EOF)) {
    if (stream_.found(TOK_EOL))
      continue;
    parse_function();
  }

  return true;
}

const parser_t::function_t *parser_t::find_function(const std::string &name) {

  for (const function_t &func : funcs_) {
    if (func.name_ == name) {
      return &func;
    }
  }
  throws("unknown function");
  return nullptr;
}

parser_t::function_t &parser_t::func() { return funcs_.back(); }

int32_t parser_t::op_type(token_e type) {
  // return precedence
  switch (type) {
  case (TOK_ADD):
  case (TOK_SUB):
    return 1;
  case (TOK_MUL):
  case (TOK_DIV):
  case (TOK_MOD):
    return 3;
  case (TOK_LT):
  case (TOK_GT):
  case (TOK_LEQ):
  case (TOK_GEQ):
  case (TOK_EQ):
    return 4;
  case (TOK_AND):
  case (TOK_OR):
    return 5;
  default:
    return 0;
  }
}

bool parser_t::is_operator() {
  token_stream_t &stream_ = ccml_.lexer().stream_;
  return op_type(stream_.type()) > 0;
}

void parser_t::parse_lhs() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;
#if 0 
    // unary minus
    if (stream_.found(TOK_SUB)) {
        // push zero
        // after parse_expr() push sub
    }
#endif
  if (stream_.found(TOK_LPAREN)) {
    parse_expr();
    stream_.found(TOK_RPAREN);
  } else if (token_t *t = stream_.found(TOK_IDENT)) {

    // call function
    if (stream_.found(TOK_LPAREN)) {
      parse_call(t);
    } else {
      asm_.emit(INS_GETV, func().find(t->str_));
    }
  } else if (token_t *t = stream_.found(TOK_VAL)) {
    asm_.emit(INS_CONST, t->val_);
  } else
    throws("expecting literal or identifier");
}

void parser_t::parse_expr_ex(uint32_t tide) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;
  parse_lhs();
  if (is_operator()) {
    token_t *op = stream_.pop();
    op_push(op->type_, tide);
    parse_expr_ex(tide);
  }
}

void parser_t::parse_expr() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;
  if (stream_.found(TOK_NOT))
    asm_.emit(INS_NOT);
  uint32_t tide = op_stack_.size();
  parse_expr_ex(tide);
  op_popall(tide);
}

void parser_t::parse_decl() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  token_t *name = stream_.pop(TOK_IDENT);
  func().ident_.push_back(name->str_);

  if (stream_.found(TOK_ASSIGN)) {
    parse_expr();
    asm_.emit(INS_SETV, func().find(name->str_));
  }
}

void parser_t::parse_assign(token_t *name) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  parse_expr();
  asm_.emit(INS_SETV, func().find(name->str_));
}

void parser_t::parse_call(token_t *name) {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  if (!stream_.found(TOK_RPAREN)) {
    do {
      parse_expr();
    } while (stream_.found(TOK_COMMA));
    stream_.pop(TOK_RPAREN);
  }
  const function_t *func = find_function(name->str_);
  if (func->sys_) {
    asm_.emit(func->sys_);
  } else {
    asm_.emit(INS_CALL, func->pos_);
  }
}

void parser_t::parse_if() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  bool has_else = false;
  // IF condition
  stream_.pop(TOK_LPAREN);
  parse_expr();
  stream_.pop(TOK_RPAREN);
  asm_.emit(INS_NOT);
  int32_t *l1 = asm_.cjmp();
  // IF body
  while (!stream_.found(TOK_END)) {
    if (stream_.found(TOK_ELSE)) {
      has_else = true;
      break;
    }
    parse_stmt();
  }
  int32_t *l2 = nullptr;
  if (has_else) {
    // skip over ELSE body
    asm_.emit(INS_CONST, 1);
    l2 = asm_.cjmp();
  }
  // ELSE body
  *l1 = asm_.pos();
  if (has_else) {
    while (!stream_.found(TOK_END)) {
      parse_stmt();
    }
    // END
    *l2 = asm_.pos();
  }
}

void parser_t::parse_while() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // top of loop
  int32_t l1 = asm_.pos();
  // WHILE condition
  stream_.pop(TOK_LPAREN);
  parse_expr();
  stream_.pop(TOK_RPAREN);
  // GOTO end if false
  asm_.emit(INS_NOT);
  int32_t *l2 = asm_.cjmp();
  // WHILE body
  while (!stream_.found(TOK_END)) {
    parse_stmt();
  }
  // unconditional jump to top
  asm_.emit(INS_CONST, 1);
  asm_.cjmp(l1);
  // WHILE end
  *l2 = asm_.pos();
}

void parser_t::parse_stmt() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  while (stream_.found(TOK_EOL)) {
    // empty
  }
  if (stream_.found(TOK_VAR)) {
    parse_decl();
  } else if (token_t *var = stream_.found(TOK_IDENT)) {
    if (stream_.found(TOK_ASSIGN)) {
      parse_assign(var);
    } else if (stream_.found(TOK_LPAREN)) {
      parse_call(var);
      // throw away the return value
      asm_.emit(INS_POP, 1);
    } else
      throw "assignment or call expected";
  } else if (stream_.found(TOK_IF)) {
    parse_if();
  } else if (stream_.found(TOK_WHILE)) {
    parse_while();
  } else if (stream_.found(TOK_RETURN)) {
    parse_expr();
    asm_.emit(INS_RET, func().frame_size());
  } else
    throw "statment expected";
  stream_.pop(TOK_EOL);
}

void parser_t::parse_function() {
  assembler_t &asm_ = ccml_.assembler();
  token_stream_t &stream_ = ccml_.lexer().stream_;

  // new function container
  funcs_.push_back(function_t());
  func().pos_ = asm_.pos();
  // parse function decl
  stream_.pop(TOK_FUNC);
  token_t *name = stream_.pop(TOK_IDENT);
  func().name_ = name->str_;
  // argument list
  stream_.pop(TOK_LPAREN);
  if (!stream_.found(TOK_RPAREN)) {
    do {
      token_t *arg = stream_.pop(TOK_IDENT);
      func().ident_.push_back(arg->str_);
    } while (stream_.found(TOK_COMMA));
    stream_.pop(TOK_RPAREN);
  }
  stream_.pop(TOK_EOL);
  // save frame index
  func().frame_ = func().ident_.size();

  // emit placeholder prologue
  asm_.emit(INS_LOCALS, 0);
  int32_t &locals = asm_.get_fixup();

  // function body
  while (!stream_.found(TOK_END)) {
    parse_stmt();
  }

  // emit dummy epilogue
  asm_.emit(INS_CONST, 0);
  asm_.emit(INS_RET, func().frame_size());

  // fixup the number of locals we are reserving
  const uint32_t num_locals = func().ident_.size() - func().frame_;
  if (num_locals)
    locals = num_locals;
}

void parser_t::op_push(token_e op, uint32_t tide) {
  while (op_stack_.size() > tide) {
    token_e top = op_stack_.back();
    if (op_type(op) > op_type(top)) {
      ccml_.assembler().emit(top);
      op_stack_.pop_back();
    } else {
      break;
    }
  }
  op_stack_.push_back(op);
}

void parser_t::op_popall(uint32_t tide) {
  while (op_stack_.size() > tide) {
    token_e op = op_stack_.back();
    ccml_.assembler().emit(op);
    op_stack_.pop_back();
  }
}

void parser_t::add_function(const std::string &name, ccml_syscall_t func) {
  function_t f;
  f.sys_ = func;
  f.name_ = name;
  funcs_.push_back(f);
}
