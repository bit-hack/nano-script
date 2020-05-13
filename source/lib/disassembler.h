#pragma once

#include "ccml.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct disassembler_t {

  disassembler_t()
    : fd_(stderr)
  {}

  void set_file(FILE *fd) {
    fd_ = fd;
  }

  // return number of bytes disassembled or <= 0 on error
  int32_t disasm(const uint8_t *ptr) const;

  // disassemble an instruction one by one
  bool disasm(program_t &prog, int32_t &index, instruction_t &out) const;

  // return instruction mnemonic
  static const char *get_mnemonic(const enum instruction_e e);

protected:
  FILE *fd_;
};

} // ccml
