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

    // kick off the code generator
    if (!codegen_->run(ast_->program, error)) {
      return false;
    }

    // collect all garbage
    ast().gc();
  }
  catch (const error_t &e) {
    error = e;
    return false;
  }
  // success
  return true;
}

void ccml_t::reset() {
  
  functions_.clear();
//  globals_.clear();
  strings_.clear();

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
