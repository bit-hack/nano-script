#include "token.h"
#include "codegen.h"
#include "errors.h"


using namespace nano;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
token_stream_t::token_stream_t(nano_t &nano)
  : ccml_(nano)
  , index_(0)
  , line_{0, 0} {
}

token_e token_stream_t::type() {
  return stream_[index_].type_;
}

const token_t *token_stream_t::found(token_e type) {
  if (stream_[index_].type_ == type)
    return pop();
  return nullptr;
}

const token_t *token_stream_t::pop(token_e type) {
  if (const token_t *t = found(type)) {
    // update the current line number
    line_ = t->line_;
    return t;
  }
  const auto &tok = stream_[index_];
  ccml_.errors().unexpected_token(tok, type);
  return nullptr;
}

const token_t *token_stream_t::pop() {
  // update the current line number
  line_ = stream_[index_].line_;
  // return the popped token
  assert(index_ < stream_.size());
  return &stream_[index_++];
}

const token_t *token_stream_t::peek() const {
  // return the token
  assert(index_ < stream_.size());
  return &stream_[index_];
}

void token_stream_t::push(const token_t &tok) {
  // check index is zero as it could cause chaos to push to an already pop'd
  // stream which would invalidate token_t* types
  assert(index_ == 0);
  stream_.push_back(tok);
}

void token_stream_t::reset() {
  index_ = 0;
  stream_.clear();
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
const char *token_t::token_name(token_e e) {
  switch (e) {
  case TOK_FUNC:     return "function";
  case TOK_END:      return "end";
  case TOK_IF:       return "if";
  case TOK_ELSE:     return "else";
  case TOK_WHILE:    return "while";
  case TOK_VAR:      return "var";
  case TOK_INT:      return "int";
  case TOK_FLOAT:    return "float";
  case TOK_FOR:      return "for";
  case TOK_TO:       return "to";
  case TOK_IDENT:    return "identifier";
  case TOK_LPAREN:   return "(";
  case TOK_RPAREN:   return ")";
  case TOK_COMMA:    return ",";
  case TOK_CONST:    return "const";
  case TOK_EOL:      return "new line";
  case TOK_ADD:      return "+";
  case TOK_SUB:      return "-";
  case TOK_MUL:      return "*";
  case TOK_DIV:      return "/";
  case TOK_MOD:      return "%";
  case TOK_AND:      return "and";
  case TOK_OR:       return "or";
  case TOK_NOT:      return "not";
  case TOK_ASSIGN:   return "=";
  case TOK_EQ:       return "==";
  case TOK_LT:       return "<";
  case TOK_GT:       return ">";
  case TOK_LEQ:      return "<=";
  case TOK_GEQ:      return ">=";
  case TOK_RETURN:   return "return";
  case TOK_EOF:      return "end of file";
  case TOK_ACC:      return "+=";
  case TOK_LBRACKET: return "[";
  case TOK_RBRACKET: return "]";
  case TOK_NONE:     return "none";
  default:
    assert(!"unhandled token type");
    return "";
  }
}
