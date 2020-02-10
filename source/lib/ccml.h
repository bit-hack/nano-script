#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <list>

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

  // name of the function
  std::string name_;

  // syscall callback or nullptr
  ccml_syscall_t sys_;

  // number of arguments syscall takes
  uint32_t sys_num_args_;

  // code address range
  uint32_t code_start_, code_end_;

  std::vector<identifier_t> locals_;
  std::vector<identifier_t> args_;

  uint32_t num_args() const {
    return is_syscall() ? sys_num_args_ : args_.size();
  }

  bool is_syscall() const {
    return sys_ != nullptr;
  }

  function_t()
    : sys_(nullptr)
    , code_start_(0)
    , code_end_(0)
  {
  }

  // syscall constructor
  function_t(const std::string &name, ccml_syscall_t sys, int32_t num_args)
      : name_(name)
      , sys_(sys)
      , sys_num_args_(num_args) {
  }

  // compiled function constructor
  function_t(const std::string &name, int32_t pos)
      : name_(name)
      , sys_(nullptr)
      , code_start_(pos)
      , code_end_(pos) {}
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

  const function_t *find_function(const std::string &name) const;
        function_t *find_function(const std::string &name);

  const function_t *find_function(const uint32_t pc) const;

  void add_function(const std::string &name, ccml_syscall_t sys, int32_t num_args);

  const std::vector<function_t> &functions() const {
    return program_.functions();
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

  bool load_source(const char *path, std::string &out);

  void add_builtins_();

  void add_(const function_t &func) {
    program_.functions().push_back(func);
  }

  program_t program_;

  std::list<std::string>           source_;
  std::list<std::string>           pending_includes_;

  std::unique_ptr<error_manager_t> errors_;
  std::unique_ptr<lexer_t>         lexer_;
  std::unique_ptr<parser_t>        parser_;
  std::unique_ptr<ast_t>           ast_;
  std::unique_ptr<codegen_t>       codegen_;
  std::unique_ptr<disassembler_t>  disassembler_;
};

} // namespace ccml
