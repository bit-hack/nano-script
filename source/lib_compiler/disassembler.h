#pragma once
#include <cstdio>

#include "ccml.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct disassembler_t {

  void dump(program_t &prog, FILE *fd);

  // return number of bytes disassembled or <= 0 on error
  int32_t disasm(const uint8_t *ptr, std::string &out) const;

  // return instruction mnemonic
  static const char *get_mnemonic(const enum instruction_e e);
};

} // ccml
