#pragma once
#include <cstring>
#include <string>
#include <array>
#include <map>

#include "ccml.h"
#include "token.h"
#include "instructions.h"
#include "ast.h"
#include "program_builder.h"


namespace ccml {

struct codegen_t {

  codegen_t(ccml_t &c, program_t &prog);

  bool run(ast_program_t &program, error_t &error);

protected:
  ccml_t &ccml_;
  program_builder_t stream_;
};

} // namespace ccml
