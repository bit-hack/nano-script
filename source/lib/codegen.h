#pragma once
#include <cstring>
#include <string>
#include <array>
#include <map>

#include "ccml.h"
#include "token.h"
#include "instructions.h"
#include "ast.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// the asm stream bridges the assembler and the code store

struct program_builder_t {

  program_builder_t(program_t &store)
    : store_(store)
    , data_(store.code_)
  {
    data_.reserve(1024);
  }

  void write8(const uint8_t data) {
    data_.push_back(data);
  }

  void write32(const uint32_t data) {
    data_.push_back((data >> 0 ) & 0xff);
    data_.push_back((data >> 8 ) & 0xff);
    data_.push_back((data >> 16) & 0xff);
    data_.push_back((data >> 24) & 0xff);
  }

  const uint8_t *data() const {
    return data_.data();
  }

  uint32_t head(int32_t adjust = 0) const {
    assert(adjust <= 0);
    assert(int32_t(data_.size()) + adjust >= 0);
    return data_.size() + adjust;
  }

  void apply_fixup(uint32_t index, int32_t value) {
    int32_t* d = (int32_t*)(data_.data() + index);
    *d = value;
  }

  uint32_t add_syscall(ccml_syscall_t syscall) {
    uint32_t index = store_.syscalls_.size();
    store_.syscalls_.push_back(syscall);
    return index;
  }

  // set the line number for the current pc
  // if line is nullptr then current lexer line is used
  void set_line(lexer_t &lexer, const token_t *line);

protected:
  program_t &store_;

  // this comes from the program_t
  std::vector<uint8_t> &data_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct instruction_t {
  instruction_e opcode;
  int32_t operand;
  const token_t *token;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct codegen_t {

  codegen_t(ccml_t &c, program_builder_t &stream);

  bool run(ast_program_t &program, error_t &error);

protected:
  ccml_t &ccml_;
  program_builder_t &stream_;
};

} // namespace ccml
