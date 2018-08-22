#pragma once
#include "ccml.h"
#include "token.h"

struct lexer_t {

  lexer_t(ccml_t &c) : ccml_(c), stream_(c), line_no_(0) {}

  bool lex(const char *source);

  ccml_t &ccml_;
  token_stream_t stream_;

protected:
  uint32_t line_no_;

  void push(token_e);
  void push_ident(const char *start, const char *end);
  void push_val(const char *start, const char *end);
};
