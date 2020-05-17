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
    line = {0, 0};
  }

  std::string error;
  line_t line;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct error_manager_t {

  error_manager_t(ccml_t &ccml): ccml_(ccml) {}

  virtual void cant_assign_const(const token_t &t) {
    on_error_(t.line_, "cant assign to constant variable '%s'", t.string());
  }

  virtual void too_many_args(const token_t &t) {
    on_error_(t.line_, "too many arguments given to '%s'", t.string());
  }

  virtual void not_enought_args(const token_t &t) {
    on_error_(t.line_, "not enough arguments given to '%s'", t.string());
  }

  virtual void array_requires_subscript(const token_t &t) {
    on_error_(t.line_, "array '%s' requires subscript []", t.string());
  }

  virtual void variable_already_declared(const token_t &t) {
    on_error_(t.line_, "variable '%s' already declared", t.string());
  }

  virtual void unexpected_token(const token_t &t) {
    on_error_(t.line_, "unexpected token '%s'", t.string());
  }

  virtual void bad_import(const token_t &t) {
    on_error_(t.line_, "unable to import '%s'", t.string());
  }

  virtual void unknown_function(const token_t &name) {
    on_error_(line_number_(), "unknown function '%s'", name.string());
  }

  virtual void unknown_identifier(const token_t &t) {
    on_error_(t.line_, "unknown identifier '%s'", t.string());
  }

  virtual void expected_func_call(const token_t &t) {
    on_error_(t.line_, "expected function call with '%s'", t.string());
  }

  virtual void unknown_variable(const token_t &t) {
    on_error_(t.line_, "unknown variable '%s'", t.string());
  }

  virtual void unknown_array(const token_t &t) {
    on_error_(t.line_, "unknown array '%s'", t.string());
  }

  virtual void expecting_lit_or_ident(const token_t &t) {
    on_error_(line_number_(),
              "expecting literal or identifier, found '%s' instead",
              t.string());
  }

  virtual void cant_assign_unknown_var(const token_t &t) {
    on_error_(t.line_, "cant assign to unknown variable '%s'",
              t.string());
  }

  virtual void equals_expected_after_operator(const token_t &t) {
    (void)t;
    on_error_(line_number_(), "equals expected after operator for compound assignment");
  }

  virtual void assign_or_call_expected_after(const token_t &t) {
    on_error_(line_number_(), "assignment or call expected after '%s'",
              t.string());
  }

  virtual void statement_expected(const token_t &t) {
    on_error_(line_number_(), "statement expected, but got '%s'", t.string());
  }

  virtual void function_already_exists(const token_t &t) {
    on_error_(t.line_, "function '%s' already exists", t.string());
  }

  virtual void var_already_exists(const token_t &t) {
    on_error_(t.line_, "var '%s' already exists in this scope", t.string());
  }

  virtual void global_already_exists(const token_t &t) {
    on_error_(t.line_, "global with name '%s' already exists", t.string());
  }

  virtual void use_of_unknown_array(const token_t &t) {
    on_error_(t.line_, "use of unknown array '%s'", t.string());
  }

  virtual void assign_to_unknown_array(const token_t &t) {
    on_error_(t.line_, "assignment to unknown array '%s'", t.string());
  }

  virtual void array_size_must_be_greater_than(const token_t &t) {
    on_error_(t.line_, "size of array '%s' must be >= 2", t.string());
  }

  virtual void variable_is_not_array(const token_t &t) {
    on_error_(t.line_, "variable '%s' was not declared as an array", t.string());
  }

  virtual void ident_is_array_not_var(const token_t &t) {
    on_error_(t.line_, "identifier '%s' an array type not variable", t.string());
  }

  virtual void cant_assign_arg(const token_t &t) {
    on_error_(t.line_, "cant assign to argument '%s'", t.string());
  }

  virtual void wrong_number_of_args(const token_t &t, int32_t takes,
                                    int32_t given) {
    on_error_(t.line_, "function '%s' takes %d arguments, %d given",
              t.string(), takes, given);
  }

  virtual void assign_to_array_missing_bracket(const token_t &t) {
    on_error_(t.line_, "assignment to array '%s' missing brackets, try '%s[...] = ...'",
              t.string(), t.string());
  }

  virtual void constant_divie_by_zero(const token_t &t) {
    on_error_(t.line_, "constant divide by zero");
  }

  virtual void global_var_const_expr(const token_t &t) {
    on_error_(t.line_, "can only assign constant expressions to globals");
  }

  virtual void const_needs_init(const token_t &t) {
    on_error_(t.line_, "constant '%s' must be initalized", t.string());
  }

  virtual void const_array_invalid(const token_t &t) {
    on_error_(t.line_, "constant arrays are not supported");
  }

  virtual void too_many_array_inits(const token_t &t, int32_t got, int32_t want) {
    on_error_(t.line_, "too many array initalizers, got %d needs %n", got, want);
  }

  virtual void array_init_in_func(const token_t &t) {
    on_error_(t.line_, "array initalizers only valid for globals");
  }

  virtual void cant_evaluate_constant(const token_t &t) {
    on_error_(t.line_, "error evaluating const expression for '%s'", t.string());
  }

  // token

  virtual void unexpected_token(const token_t &t, token_e e) {
    on_error_(t.line_, "unexpected token '%s' expecting '%s'", t.string(),
              token_t::token_name(e));
  }

  virtual void bad_array_init_value(const token_t &t) {
    on_error_(t.line_, "bad array initalizer value '%s'", t.string());
  }

  // assembler

  virtual void program_too_large() {
    on_error_(line_number_(), "program too large, ran out of space");
  }

  // lexer

  virtual void unexpected_character(const line_t line, const char ch) {
    on_error_(line, "unexpected character '%c' in source", ch);
  }

  virtual void string_quote_mismatch(const line_t line) {
    on_error_(line, "string missing closing quote \"");
  }

protected:
  line_t line_number_() const {
    return ccml_.lexer().stream().line_number();
  }

  void on_error_(line_t line, const char *fmt, ...);

  ccml_t &ccml_;
};

} // namespace ccml
