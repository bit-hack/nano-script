#pragma once

#include <string>

#include "ccml.h"
#include "lexer.h"
#include "token.h"


struct ccml_error_t {
  std::string error;
  uint32_t line;
};

struct error_manager_t {

  error_manager_t(ccml_t &ccml) : ccml_(ccml) {}

  // parser

  virtual void unexpected_token(const token_t &t) {
    on_error_(t.line_no_, "unexpected token '%s'", t.str_.c_str());
  }

  virtual void unknown_function(const token_t &name) {
    on_error_(line_number_(), "unknown function '%s'", name.str_.c_str());
  }

  virtual void unknown_identifier(const token_t &t) {
    on_error_(t.line_no_, "unknown identifier '%s'", t.str_.c_str());
  }

  virtual void expecting_lit_or_ident(const token_t &t) {
    on_error_(line_number_(),
              "expecting literal or identifier, found '%s' instead",
              t.str_.c_str());
  }

  virtual void cant_assign_unknown_var(const token_t &t) {
    on_error_(t.line_no_, "cant assign to unknown variable '%s'",
              t.str_.c_str());
  }

  virtual void assign_or_call_expected_after(const token_t &t) {
    on_error_(line_number_(), "assignment or call expected after '%s'",
              t.str_.c_str());
  }

  virtual void statement_expected() {
    on_error_(line_number_(), "statement expected");
  }

  virtual void function_already_exists(const token_t &name) {
    on_error_(name.line_no_, "function '%s' already exists", name.str_.c_str());
  }

  // token

  virtual void unexpected_token(const token_t &t, token_e e) {
    static const char *expected = token_t::token_name(e);
    on_error_(t.line_no_, "unexpected token '%s' expecting '%s'",
              t.str_.c_str(), expected);
  }

  // assembler

  virtual void program_too_large() {
    on_error_(line_number_(), "program too large, ran out of space");
  }

  // lexer

  virtual void unexpected_character(const uint32_t line, const char ch) {
    on_error_(line, "unexpected character '%c' in source", ch);
  }

protected:
  uint32_t line_number_() const {
    return ccml_.lexer().stream_.line_number();
  }

  void on_error_(uint32_t line, const char *fmt, ...);

  ccml_t &ccml_;
};
