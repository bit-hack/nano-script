#include <cstdarg>
#include <memory>

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
ccml_t::ccml_t()
  : optimize(true)
  , program_()
  , errors_(new error_manager_t(*this))
  , lexer_(new lexer_t(*this))
  , parser_(new parser_t(*this))
  , ast_(new ast_t(*this))
  , codegen_(new codegen_t(*this, program_.stream()))
  , disassembler_(new disassembler_t(*this))
  
{
  add_builtins_();
}

bool ccml_t::build(const char *source, error_t &error) {

  // store the input program internaly
  source_.emplace_back(source);
  const std::string *input = &source_.back();

  // clear the error
  error.clear();
  // lex into tokens
  try {

    for (;;) {

      // lex the input into tokens
      if (!lexer_->lex(input->c_str())) {
        return false;
      }
      // parse into instructions
      if (!parser_->parse(error)) {
        return false;
      }
      // break if there are no includes to process
      if (pending_includes_.empty()) {
        break;
      }

      // lookup the next include file
      const std::string &path = pending_includes_.back();
      source_.emplace_back();
      if (!load_source(path.c_str(), source_.back())) {
        // XXX: set error status to say its an include failure
        return false;
      }
      input = &source_.back();
      pending_includes_.pop_back();
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
  program_.reset();
}

function_t *ccml_t::find_function(const std::string &name) {
  for (auto &f : program_.functions()) {
    if (f.name_ == name) {
      return &f;
    }
  }
  return nullptr;
}

const function_t *ccml_t::find_function(const std::string &name) const {
  for (const auto &f : program_.functions()) {
    if (f.name_ == name) {
      return &f;
    }
  }
  return nullptr;
}

const function_t *ccml_t::find_function(const uint32_t pc) const {
  // XXX: accelerate this with a map later please!
  for (const auto &f : program_.functions()) {
    if (pc >= f.code_start_ && pc < f.code_end_) {
      return &f;
    }
  }
  return nullptr;
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

bool ccml_t::load_source(const char *path, std::string &out) {

  FILE *fd = fopen(path, "rb");
  if (!fd) {
    return false;
  }

  fseek(fd, 0, SEEK_END);
  size_t size = size_t(ftell(fd));
  fseek(fd, 0, SEEK_SET);
  if (size <= 0) {
    fclose(fd);
    return false;
  }

  out.resize(size + 1);
  auto temp = std::make_unique<char[]>( size + 1 );
  if (fread(temp.get(), 1, size, fd) != size) {
    fclose(fd);
    return false;
  }
  temp.get()[size] = '\0';

  out = temp.get();

  fclose(fd);
  return true;
}
