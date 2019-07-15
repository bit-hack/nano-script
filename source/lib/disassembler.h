#pragma once

#include "ccml.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct disassembler_t {

  disassembler_t(ccml_t &ccml)
    : ccml_(ccml)
    , fd_(stderr)
  {}

  void set_file(FILE *fd) {
    fd_ = fd;
  }

  // return number of bytes disassembled or <= 0 on error
  int32_t disasm(const uint8_t *ptr) const;

  // return number of bytes disassembled or <= 0 on error
  int32_t disasm();

  // disassemble an instruction one by one
  bool disasm(int32_t &index, instruction_t &out) const;

  // return instruction mnemonic
  static const char *get_mnemonic(const enum instruction_e e);

protected:
  ccml_t &ccml_;
  FILE *fd_;
};

} // ccml
