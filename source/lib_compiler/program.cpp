#include "codegen.h"
#include "program.h"

namespace ccml {

program_t::program_t()
  : builder_(new program_builder_t{*this}) {}

void program_t::reset() {
  line_table_.clear();
  strings_.clear();
  builder_.reset(new program_builder_t(*this));
  functions_.clear();
}

const function_t *program_t::function_find(const std::string &name) const {
  for (const function_t &f : functions_) {
    if (f.name() == name) {
      return &f;
    }
  }
  return nullptr;
}

function_t *program_t::function_find(const std::string &name) {
  for (function_t &f : functions_) {
    if (f.name() == name) {
      return &f;
    }
  }
  return nullptr;
}

function_t *program_t::function_find(uint32_t pc) {
  for (function_t &f : functions_) {
    if (pc >= f.code_start_ && pc < f.code_end_) {
      return &f;
    }
  }
  return nullptr;
}

} // namespace ccml
