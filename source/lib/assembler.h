#pragma once
#include <cstring>
#include <string>
#include <array>
#include <map>

#include "ccml.h"
#include "token.h"


enum instruction_e {
  INS_ADD,
  INS_SUB,
  INS_MUL,
  INS_DIV,
  INS_MOD,
  INS_AND,
  INS_OR,
  INS_NOT,

  INS_LT,
  INS_GT,
  INS_LEQ,
  INS_GEQ,
  INS_EQ,

  INS_JMP,      // conditional jump to code offset
  INS_CALL,     // call a function
  INS_RET,      // return to previous frame {popping locals and args}
  INS_POP,      // pop constant from stack
  INS_CONST,    // push constant

  INS_GETV,     // get local
  INS_SETV,     // set local

  INS_NOP,      // no operation

  INS_SCALL,    // system call
                // TODO: dont encode a pointer in the operand!

  INS_LOCALS,   // number of locals to reserve on the stack

  INS_GETG,     // get global
  INS_SETG,     // set global

  __INS_COUNT__,  // number of instructions
};

struct assembler_t {

  assembler_t(ccml_t &c);

  void     emit(uint32_t line, token_e);
  void     emit(uint32_t line, instruction_e ins);
  int32_t *emit(uint32_t line, instruction_e ins, int32_t v);
  void     emit(uint32_t line, ccml_syscall_t sys);

  int32_t pos() const;

  int32_t disasm(const uint8_t *ptr) const;
  int32_t disasm();

  // return a reference to the last instructions operand
  int32_t &get_fixup();

  const uint8_t *get_code() const {
    return code_.data();
  }

protected:
  void write8(uint8_t v);
  void write32(int32_t v);

  std::map<const uint8_t *, uint32_t> line_table_;

  ccml_t &ccml_;
  uint32_t head_;
  std::array<uint8_t, 1024> code_;
};
