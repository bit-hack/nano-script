#pragma once
#include "nano.h"
#include "token.h"


namespace nano {

struct lexer_t {

  lexer_t(nano_t &c)
    : nano_(c)
    , stream_(c)
    , line_{0, 1}
  {}

  // lex input sourcecode
  bool lex(const char *source, int32_t file_no);

  // reset any stored state
  void reset();

  token_stream_t &stream() {
    return stream_;
  }

protected:
  nano_t &nano_;

  // stream of parsed tokens
  token_stream_t stream_;

  // current line number at this point in lexing
  line_t line_;

  void push_(token_e);
  void push_ident_(const char *start, const char *end);
  void push_val_(const char *start, const char *end);
  void push_string_(const char *start, const char *end);
};

} // namespace nano
