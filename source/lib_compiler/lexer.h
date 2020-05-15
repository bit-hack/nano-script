#pragma once
#include "ccml.h"
#include "token.h"


namespace ccml {

struct lexer_t {

  lexer_t(ccml_t &c)
    : ccml_(c)
    , stream_(c)
    , line_no_(1)
  {}

  // lex input sourcecode
  bool lex(const char *source);

  // reset any stored state
  void reset();

  token_stream_t &stream() {
    return stream_;
  }

protected:
  ccml_t &ccml_;

  // stream of parsed tokens
  token_stream_t stream_;

  // current line number being parsed
  uint32_t line_no_;

  void push_(token_e);
  void push_ident_(const char *start, const char *end);
  void push_val_(const char *start, const char *end);
  void push_string_(const char *start, const char *end);
};

} // namespace ccml
