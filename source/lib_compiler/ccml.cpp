#include <cstdarg>

#include "ccml.h"

#include "errors.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "disassembler.h"
#include "phases.h"
#include "source.h"

using namespace nano;

nano_t::nano_t(program_t &prog)
  : optimize(true)
  , program_(prog)
  , sources_(nullptr)
  , source_(nullptr)
  , errors_(new error_manager_t(*this))
  , parser_(new parser_t(*this))
  , ast_(new ast_t(*this))
  , codegen_(new codegen_t(*this, program_))
{
}

bool nano_t::build(source_manager_t &sources, error_t &error) {

  // we need something to compile
  if (sources.count() == 0) {
    return false;
  }

  // store the source manager for now so we can import into it
  sources_ = &sources;

  // clear the error
  error.clear();
  try {

    // lex and parse all provided sources
    int32_t index = 0;
    for (; index < sources.count(); ++index) {
      // find the next uncompiled source
      const source_t &src = sources.get_source(index);
      source_ = &src;
      // create a lexer for this file
      lexer_.emplace_back(new lexer_t(*this));
      // lex this source file into tokens
      if (!lexer().lex(src.data(), index)) {
        return false;
      }
      // parse into instructions
      if (!parser_->parse(error)) {
        return false;
      }
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

void nano_t::reset() {
  lexer_.clear();
  parser_->reset();
  ast_->reset();
  program_.reset();
}

void nano_t::syscall_register(const std::string &name, int32_t num_args) {
  ast_decl_func_t *func = ast_->alloc<ast_decl_func_t>(name);
  func->is_syscall = true;
  for (int32_t i = 0; i < num_args; ++i) {
    func->args.emplace_back(nullptr);
  }
  auto &prog = ast_->program;
  prog.children.push_back(func);
}
