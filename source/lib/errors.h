#pragma once

#include <string>

#include "ccml.h"
#include "lexer.h"
#include "token.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct error_t {

  void clear() {
    error.clear();
    line = 0;
  }

  std::string error;
  uint32_t line;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct error_manager_t {

  error_manager_t(ccml_t &ccml): ccml_(ccml) {}

  virtual void too_many_args(const token_t &t) {
    on_error_(t.line_no_, "too many arguments given to '%s'", t.string());
  }

  virtual void not_enought_args(const token_t &t) {
    on_error_(t.line_no_, "not enough arguments given to '%s'", t.string());
  }

  virtual void array_requires_subscript(const token_t &t) {
    on_error_(t.line_no_, "array '%s' requires subscript []", t.string());
  }

  virtual void variable_already_declared(const token_t &t) {
    on_error_(t.line_no_, "variable '%s' already declared", t.string());
  }

  virtual void unexpected_token(const token_t &t) {
    on_error_(t.line_no_, "unexpected token '%s'", t.string());
  }

  virtual void unknown_function(const token_t &name) {
    on_error_(line_number_(), "unknown function '%s'", name.str_.c_str());
  }

  virtual void unknown_identifier(const token_t &t) {
    on_error_(t.line_no_, "unknown identifier '%s'", t.str_.c_str());
  }

  virtual void expected_func_call(const token_t &t) {
    on_error_(t.line_no_, "expected function call with '%s'", t.str_.c_str());
  }

  virtual void unknown_variable(const token_t &t) {
    on_error_(t.line_no_, "unknown variable '%s'", t.str_.c_str());
  }

  virtual void unknown_array(const token_t &t) {
    on_error_(t.line_no_, "unknown array '%s'", t.str_.c_str());
  }

  virtual void expecting_lit_or_ident(const token_t &t) {
    on_error_(line_number_(),
              "expecting literal or identifier, found '%s' instead",
              t.string());
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

  virtual void function_already_exists(const token_t &t) {
    on_error_(t.line_no_, "function '%s' already exists", t.str_.c_str());
  }

  virtual void var_already_exists(const token_t &t) {
    on_error_(t.line_no_, "var '%s' already exists in this scope", t.str_.c_str());
  }

  virtual void global_already_exists(const token_t &t) {
    on_error_(t.line_no_, "global with name '%s' already exists", t.str_.c_str());
  }

  virtual void use_of_unknown_array(const token_t &t) {
    on_error_(t.line_no_, "use of unknown array '%s'", t.str_.c_str());
  }

  virtual void assign_to_unknown_array(const token_t &t) {
    on_error_(t.line_no_, "assignment to unknown array '%s'", t.str_.c_str());
  }

  virtual void array_size_must_be_greater_than(const token_t &t) {
    on_error_(t.line_no_, "size of array '%s' must be >= 2", t.str_.c_str());
  }

  virtual void variable_is_not_array(const token_t &t) {
    on_error_(t.line_no_, "variable '%s' was not declared as an array", t.str_.c_str());
  }

  virtual void ident_is_array_not_var(const token_t &t) {
    on_error_(t.line_no_, "identifier '%s' an array type not variable", t.str_.c_str());
  }

  virtual void wrong_number_of_args(const token_t &t, int32_t takes,
                                    int32_t given) {
    on_error_(t.line_no_, "function '%s' takes %d arguments, %d given",
              t.str_.c_str(), takes, given);
  }

  virtual void assign_to_array_missing_bracket(const token_t &t) {
    on_error_(t.line_no_, "assignment to array '%s' missing brackets, try '%s[...] = ...'",
              t.str_.c_str(), t.str_.c_str());
  }

  virtual void constant_divie_by_zero(const token_t &t) {
    on_error_(t.line_no_, "constant divide by zero");
  }

  // token

  virtual void unexpected_token(const token_t &t, token_e e) {
    on_error_(t.line_no_, "unexpected token '%s' expecting '%s'", t.string(),
              token_t::token_name(e));
  }

  // assembler

  virtual void program_too_large() {
    on_error_(line_number_(), "program too large, ran out of space");
  }

  // lexer

  virtual void unexpected_character(const uint32_t line, const char ch) {
    on_error_(line, "unexpected character '%c' in source", ch);
  }

  virtual void string_quote_mismatch(const uint32_t line, const char ch) {
    on_error_(line, "string missing closing quote \"");
  }

protected:
  uint32_t line_number_() const {
    return ccml_.lexer().stream_.line_number();
  }

  void on_error_(uint32_t line, const char *fmt, ...);

  ccml_t &ccml_;
};

} // namespace ccml
