#include <cstdarg>

#include "ccml.h"

#include "errors.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "disassembler.h"
#include "vm.h"
#include "phases.h"

using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
ccml_t::ccml_t(program_t &prog)
  : optimize(true)
  , program_(prog)
  , errors_(new error_manager_t(*this))
  , lexer_(new lexer_t(*this))
  , parser_(new parser_t(*this))
  , ast_(new ast_t(*this))
  , codegen_(new codegen_t(*this, program_.builder()))
{
  add_builtins_();
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
    // run pre-codegen passes
    run_pre_codegen(*this);

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
  lexer_->reset();
  parser_->reset();
  ast_->reset();

  // XXX: do we want to do this now?
  program_.reset();
}

void ccml_t::add_function(const std::string &name, ccml_syscall_t sys, int32_t num_args) {
  ast_decl_func_t *func = ast_->alloc<ast_decl_func_t>(name);
  func->syscall = sys;
  for (int32_t i = 0; i < num_args; ++i) {
    func->args.emplace_back(nullptr);
  }
  auto &prog = ast_->program;
  prog.children.push_back(func);
}
