#pragma once
#include "ccml.h"
#include "token.h"


namespace ccml {

struct lexer_t {

  lexer_t(ccml_t &c)
    : stream_(c)
    , ccml_(c)
    , line_no_(0) {}

  // lex input sourcecode
  bool lex(const char *source);

  // reset any stored state
  void reset();

  // return a parse source line
  const std::string &get_line(line_t no) const {
    const auto line = no.line;
    static const std::string empty;
    if (line >= 0 && line < int32_t(lines_.size())) {
      return lines_.at(line);
    }
    return empty;
  }

  token_stream_t &stream() {
    return stream_;
  }

protected:
  ccml_t &ccml_;

  // stream of parsed tokens
  token_stream_t stream_;

  // current line number being parsed
  uint32_t line_no_;

  // line table
  std::vector<std::string> lines_;

  void push_(token_e);
  void push_ident_(const char *start, const char *end);
  void push_val_(const char *start, const char *end);
  void push_string_(const char *start, const char *end);
};

} // namespace ccml
