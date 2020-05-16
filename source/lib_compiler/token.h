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
  TOK_INT,
  TOK_FLOAT,
  TOK_FOR,
  TOK_TO,
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
  TOK_CONST,
  TOK_IMPORT
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct token_t {

  token_t(token_e t, line_t line)
    : type_(t)
    , str_()
    , line_(line)
    , val_(0) {}

  token_t(token_e tok, const char *s, line_t line)
    : type_(tok)
    , str_(s)
    , line_(line)
    , val_(0) {}

  token_t(token_e tok, const std::string &s, line_t line)
    : type_(tok)
    , str_(s)
    , line_(line)
    , val_(0) {}

  token_t(const int32_t &v, line_t line)
    : type_(TOK_INT)
    , line_(line)
    , val_(v) {}

  token_t(const float &v, line_t line)
    : type_(TOK_FLOAT)
    , line_(line)
    , valf_(v) {}

  // convert a token_e to a token string
  static const char *token_name(token_e e);

  float get_float() const {
    assert(type_ == TOK_FLOAT);
    return valf_;
  }

  int32_t get_int() const {
    assert(type_ == TOK_INT);
    return val_;
  }

  // stringify this token
  const char *string() const {
    if (type_ == TOK_STRING) {
      return str_.empty() ? "" : str_.c_str();
    }
    else {
      return str_.empty() ? token_name(type_) : str_.c_str();
    }
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
  line_t line_;

  union {
    int32_t val_;
    float valf_;
  };
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

  // return next token from stream without removing it
  const token_t *peek() const;

  // push a token onto the stream
  // note: this can invalidate all token_t* types
  void push(const token_t &tok);

  // reset all token stream state
  void reset();

  // return the current line number of the token stream
  line_t line_number() const {
    return line_;
  }

protected:
  ccml_t &ccml_;

  uint32_t index_;

  // tracks the line of the last token popped from the stream
  line_t line_;

  std::vector<token_t> stream_;
};

} // namespace ccml
