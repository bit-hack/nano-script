#pragma once
#include <stdint.h>
#include <string>
#include <vector>

#include "ccml.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
enum token_e {
  TOK_FUNC,
  TOK_END,
  TOK_IF,
  TOK_ELSE,
  TOK_WHILE,
  TOK_VAR,
  TOK_VAL,
  TOK_IDENT,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_LBRACKET,
  TOK_RBRACKET,
  TOK_COMMA,
  TOK_EOL,
  TOK_ADD,
  TOK_SUB,
  TOK_MUL,
  TOK_DIV,
  TOK_MOD,
  TOK_AND,
  TOK_OR,
  TOK_NOT,
  TOK_ASSIGN,
  TOK_EQ,
  TOK_LT,
  TOK_GT,
  TOK_LEQ,
  TOK_GEQ,
  TOK_RETURN,
  TOK_EOF,
  TOK_ACC,
  TOK_STRING,
  TOK_NONE,
  // artificial token generated during expression parser
  TOK_NEG,
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct token_t {

  token_t(token_e t, uint32_t line)
    : type_(t)
    , str_()
    , val_(0)
    , line_no_(line) {}

  token_t(token_e tok, const char *s, uint32_t line)
    : type_(tok)
    , str_(s)
    , val_(0)
    , line_no_(line) {}

  token_t(token_e tok, const std::string &s, uint32_t line)
    : type_(tok)
    , str_(s)
    , val_(0)
    , line_no_(line) {}

  token_t(int32_t v, uint32_t line)
    : type_(TOK_VAL)
    , val_(v)
    , line_no_(line) {}

  // convert a token_e to a token string
  static const char *token_name(token_e e);

  // stringify this token
  const char *string() const {
    return str_.empty() ? token_name(type_) : str_.c_str();
  }

  bool is_binary_op() const {
    switch (type_) {
    case TOK_ADD:
    case TOK_SUB:
    case TOK_MUL:
    case TOK_DIV:
    case TOK_MOD:
    case TOK_AND:
    case TOK_OR:
    case TOK_EQ:
    case TOK_LT:
    case TOK_GT:
    case TOK_LEQ:
    case TOK_GEQ:
      return true;
    default:
      return false;
    }
  }

  bool is_unary_op() const {
    switch (type_) {
    case TOK_NOT:
    case TOK_NEG:
      return true;
    default:
      return false;
    }
  }

  bool is_operator() const {
    return is_binary_op() || is_unary_op();
  }

  token_e type_;
  std::string str_;
  int32_t val_;
  uint32_t line_no_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct token_stream_t {

  token_stream_t(ccml_t &ccml);

  // return the type of the next token in the stream
  token_e type();

  // return a token if it is the next in the stream
  const token_t *found(token_e type);

  // pop a token expecting it to be a certain type
  const token_t *pop(token_e type);

  // pop the next token from the stream
  const token_t *pop();

  // push a token onto the stream
  // note: this can invalidate all token_t* types
  void push(const token_t &tok);

  // reset all token stream state
  void reset();

  // return the current line number of the token stream
  uint32_t line_number() const {
    return line_no_;
  }

protected:
  ccml_t &ccml_;

  uint32_t index_;
  uint32_t line_no_;
  std::vector<token_t> stream_;
};

} // namespace ccml
