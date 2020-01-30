#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <map>

#include "common.h"
#include "program.h"
#include "value.h"
#include "instructions.h"
#include "thread_error.h"


namespace ccml {

struct identifier_t {
  std::string name_;
  int32_t offset_;
};

struct function_t {
  std::string name_;
  ccml_syscall_t sys_;

  uint32_t num_args_;

  // code address range
  uint32_t code_start_;
  uint32_t code_end_;

  std::vector<identifier_t> locals_;
  std::vector<identifier_t> args_;

  bool is_syscall() const {
    return sys_ != nullptr;
  }

  function_t()
    : sys_(nullptr)
    , num_args_(0)
    , code_start_(0)
    , code_end_(0)
  {
  }

  function_t(const std::string &name, ccml_syscall_t sys, int32_t num_args)
      : name_(name), sys_(sys), num_args_(num_args), code_start_(~0u),
        code_end_(~0u) {}

  function_t(const std::string &name, int32_t pos, int32_t num_args)
      : name_(name), sys_(nullptr), num_args_(num_args), code_start_(pos),
        code_end_(~0u) {}
};


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct ccml_t {

  ccml_t();

  // accessors
  error_manager_t &errors()       { return *errors_; }
  program_t       &program()      { return program_; }
  lexer_t         &lexer()        { return *lexer_; }
  parser_t        &parser()       { return *parser_; }
  ast_t           &ast()          { return *ast_; }
  codegen_t       &codegen()      { return *codegen_; }
  disassembler_t  &disassembler() { return *disassembler_; }

  bool build(const char *source, error_t &error);

  void reset();

  const uint8_t *code() const {
    return program_.data();
  }

  const function_t *find_function(const std::string &name) const;
  const function_t *find_function(const uint32_t pc) const;
  function_t *find_function(const std::string &name);

  void add_function(const std::string &name, ccml_syscall_t sys, int32_t num_args);

  const std::vector<function_t> &functions() const {
    return functions_;
  }

  const std::vector<std::string> &strings() const {
    return program_.strings();
  }

  // enable codegen optimizations
  bool optimize;

protected:
  friend struct lexer_t;
  friend struct parser_t;
  friend struct codegen_t;
  friend struct disassembler_t;
  friend struct token_stream_t;
  friend struct error_manager_t;
  friend struct pregen_functions_t;
  friend struct codegen_pass_t;

  void add_builtins_();

  void add_(const function_t &func) {
    functions_.push_back(func);
  }

  void add_(const std::string &string) {
    program_.strings().push_back(string);
  }

  program_t program_;
  std::vector<function_t> functions_;

  std::unique_ptr<error_manager_t> errors_;
  std::unique_ptr<lexer_t>         lexer_;
  std::unique_ptr<parser_t>        parser_;
  std::unique_ptr<ast_t>           ast_;
  std::unique_ptr<codegen_t>       codegen_;
  std::unique_ptr<disassembler_t>  disassembler_;
};

} // namespace ccml
