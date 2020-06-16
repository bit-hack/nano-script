#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <map>

#include "../lib_common/common.h"
#include "../lib_common/program.h"
#include "../lib_common/instructions.h"
#include "../lib_common/types.h"
#include "../lib_common/source.h"


namespace nano {

struct nano_t {

  nano_t(program_t &prog);

  // accessors
  error_manager_t &errors()       { return *errors_;  }
  lexer_t         &lexer()        { return *lexer_.back(); }
  parser_t        &parser()       { return *parser_;  }
  ast_t           &ast()          { return *ast_;     }
  codegen_t       &codegen()      { return *codegen_; }

  bool build(source_manager_t &sources, error_t &error);

  void reset();

  // register a system call
  //
  // note: if num_args < 0 it can take a variable number of arguments
  void syscall_register(const std::string &name, int32_t num_args);

  // enable codegen optimizations
  bool optimize;

protected:
  friend struct lexer_t;
  friend struct parser_t;
  friend struct codegen_t;
  friend struct error_manager_t;
  friend struct pregen_functions_t;
  friend struct codegen_pass_t;

  // the current program we are building
  program_t &program_;

  // the source manager we are working with
  source_manager_t *sources_;
  // the current file we are parsing
  const source_t *source_;

  // objects used while compiling
  std::unique_ptr<error_manager_t> errors_;

  // stack of lexers, one per file
  std::vector<std::unique_ptr<lexer_t>> lexer_;

  std::unique_ptr<parser_t>  parser_;
  std::unique_ptr<ast_t>     ast_;
  std::unique_ptr<codegen_t> codegen_;
};

} // namespace nano
