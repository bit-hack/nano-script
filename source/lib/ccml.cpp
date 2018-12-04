#include <cstdarg>

#include "ccml.h"

#include "errors.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "disassembler.h"
#include "vm.h"
#include "sema.h"
#include "optimize.h"


using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
ccml_t::ccml_t()
  : store_()
  , lexer_(new lexer_t(*this))
  , parser_(new parser_t(*this))
  , ast_(new ast_t(*this))
  , codegen_(new codegen_t(*this, store_.stream()))
  , disassembler_(new disassembler_t(*this))
  , vm_(new vm_t(*this))
  , errors_(new error_manager_t(*this))
{
  add_builtins_();
}

ccml_t::~ccml_t() {
}

bool ccml_t::build(const char *source, error_t &error) {
  // clear the error
  error.clear();
  // lex into tokens
  try {
    if (!lexer_->lex(source)) {
      return false;
    }
    // parse into instructions
    if (!parser_->parse(error)) {
      return false;
    }

    // run semantic checker
    run_sema(*this);

    // run optmizer
    run_optimize(*this);

    // XXX: we need a sema stage
    // kick off the code generator
    if (!codegen_->run(ast_->program, error)) {
      return false;
    }

    // collect all garbage
    ast().gc();

//    disassembler().disasm();
  }
  catch (const error_t &e) {
    error = e;
    return false;
  }
  // success
  return true;
}

void ccml_t::reset() {
  lexer_->reset();
  parser_->reset();
  ast_->reset();
  codegen_->reset();
  vm_->reset();
  store_.reset();
}

const function_t *ccml_t::find_function(const std::string &name) const {
  for (const auto &f : functions_) {
    if (f.name_ == name) {
      return &f;
    }
  }
  return nullptr;
}

void ccml_t::add_function(const std::string &name, ccml_syscall_t sys, int32_t num_args) {
  const function_t fn{name, sys, num_args};
  functions_.push_back(fn);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
code_store_t::code_store_t()
  : stream_(new asm_stream_t{*this}) {
}

void code_store_t::reset() {
  line_table_.clear();
  stream_.reset(new asm_stream_t(*this));
}

bool code_store_t::active_vars(const uint32_t pc,
                               std::vector<const identifier_t *> &out) const {
  // TODO
  return true;
}

namespace ccml {
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
value_t value_add(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return value_t{a + b};
#else
  return value_t{a.v + b.v};
#endif
}

value_t value_sub(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return value_t{a - b};
#else
  return value_t{a.v - b.v};
#endif
}

value_t value_mul(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return value_t{a * b};
#else
  return value_t{a.v * b.v};
#endif
}

bool value_div(const value_t &a, const value_t &b, value_t &r) {
#if USE_OLD_VALUE_TYPE
  if (b == 0) {
    return false;
  } else {
    r = value_t{a / b};
    return true;
  }
#else
  if (b.v == 0) {
    return false;
  } else {
    r = value_t{a.v / b.v};
    return true;
  }
#endif
}

bool value_mod(const value_t &a, const value_t &b, value_t &r) {
#if USE_OLD_VALUE_TYPE
  if (b == 0) {
    return false;
  } else {
    r = value_t{a % b};
    return true;
  }
#else
  if (b.v == 0) {
    return false;
  } else {
    r = value_t{a.v % b.v};
    return true;
  }
#endif
}

value_t value_and(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return value_t{a && b};
#else
  return value_t{a.v && b.v};
#endif
}

value_t value_or(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return value_t{a || b};
#else
  return value_t{a.v || b.v};
#endif
}

value_t value_not(const value_t &a) {
#if USE_OLD_VALUE_TYPE
  return value_t{!a};
#else
  return value_t{!a.v};
#endif
}

value_t value_neg(const value_t &a) {
#if USE_OLD_VALUE_TYPE
  return value_t{-a};
#else
  return value_t{-a.v};
#endif
}

bool value_lt(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return a < b;
#else
  return a.v < b.v;
#endif
}

bool value_gt(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return a > b;
#else
  return a.v > b.v;
#endif
}

bool value_leq(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return a <= b;
#else
  return a.v <= b.v;
#endif
}

bool value_geq(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return a >= b;
#else
  return a.v >= b.v;
#endif
}

bool value_eq(const value_t &a, const value_t &b) {
#if USE_OLD_VALUE_TYPE
  return a == b;
#else
  return a.v == b.v;
#endif
}

int32_t value_to_int(const value_t &a) {
#if USE_OLD_VALUE_TYPE
  return int32_t(a);
#else
  return int32_t(a.v);
#endif
}

value_t value_from_int(const int32_t &a) {
#if USE_OLD_VALUE_TYPE
  return value_t{a};
#else
  return value_t{a};
#endif
}

} // namespace ccml
