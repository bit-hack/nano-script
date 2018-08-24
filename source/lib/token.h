#pragma once
#include "ccml.h"
#include <stdint.h>
#include <string>
#include <vector>

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
};

struct token_t {

  token_t(token_e t, uint32_t line)
    : type_(t)
    , str_()
    , val_(0)
    , line_no_(line)
  {}

  token_t(const char *s, uint32_t line)
    : type_(TOK_IDENT)
    , str_(s)
    , val_(0)
    , line_no_(line)
  {}

  token_t(const std::string &s, uint32_t line)
    : type_(TOK_IDENT)
    , str_(s)
    , val_(0)
    , line_no_(line)
  {}

  token_t(int32_t v, uint32_t line)
    : type_(TOK_VAL)
    , val_(v)
    , line_no_(line)
  {}

  token_e type_;
  std::string str_;
  int32_t val_;
  uint32_t line_no_;
};

struct token_stream_t {

  token_stream_t(ccml_t &ccml)
    : ccml_(ccml)
    , index_(0)
    , line_no_(0) {
  }

  token_e type() {
    return stream_[index_].type_;
  }

  token_t *found(token_e type) {
    if (stream_[index_].type_ == type)
      return pop();
    return nullptr;
  }

  token_t *pop(token_e type) {
    // XXX: Token to string
    if (token_t *t = found(type))
      return t;
    const auto &tok = stream_[index_];
    ccml_.on_error_(
      tok.line_no_,
      "unexpected token '%d' expecting '%d'",
      tok.type_,
      type);
    return nullptr;
  }

  token_t *pop() {
    line_no_ = stream_[index_].line_no_;
    return &stream_[index_++];
  }

  void push(const token_t &tok) {
    stream_.push_back(tok);
  }

  void reset() {
    index_ = 0;
    stream_.clear();
  }

  ccml_t &ccml_;
  uint32_t index_;
  uint32_t line_no_;
  std::vector<token_t> stream_;
};
