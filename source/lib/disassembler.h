#pragma once

#include "ccml.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct disassembler_t {

  disassembler_t(ccml_t &ccml)
    : ccml_(ccml)
  {}

  // return number of bytes disassembled or <= 0 on error
  int32_t disasm(const uint8_t *ptr) const;

  // return number of bytes disassembled or <= 0 on error
  int32_t disasm();

  // return instruction mnemonic
  static const char *get_mnemonic(const enum instruction_e e);

protected:
  ccml_t &ccml_;
};

} // ccml
