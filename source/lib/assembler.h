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

  INS_JMP,      // unconditional jump
  INS_CJMP,     // conditional jump to code offset
  INS_CALL,     // call a function
  INS_RET,      // return to previous frame {popping locals and args}

  // system call
  INS_SCALL,

  // pop constant:
  //    pop()
  INS_POP,

  // push constant:
  //    push( operand )
  INS_CONST,

  // reserve locals in stack frame
  INS_LOCALS,

  // get local:
  //    push( stack[ fp + operand ] )
  INS_GETV,

  // set local:
  //    v = pop()
  //    stack[ fp + operand ] = v
  INS_SETV,

  // get local indexed:
  //    i = pop()
  //    push( stack[ fp + operand + i ] )
  INS_GETVI,

  // set local indexed
  //    v = pop()
  //    i = pop()
  //    stack[ fp + operand + i ] = v
  INS_SETVI,

  // get global
  //    push( stack[ operand ] )
  INS_GETG,

  // set local:
  //    v = pop()
  //    stack[ operand ] = v
  INS_SETG,

  // get global indexed:
  //    i = pop()
  //    push( stack[ operand + i ] )
  INS_GETGI,

  // set global indexed:
  //    v = pop()
  //    i = pop()
  //    stack[ operand + i ] = v
  INS_SETGI,

  __INS_COUNT__,  // number of instructions
};

struct assembler_t {

  assembler_t(ccml_t &c);

  // emit into the code stream
  void     emit(token_e                     , const token_t *t = nullptr);
  void     emit(instruction_e ins           , const token_t *t = nullptr);
  int32_t *emit(instruction_e ins, int32_t v, const token_t *t = nullptr);

  // return the current output head
  int32_t pos() const;

  // disassemble a code stream
  int32_t disasm(const uint8_t *ptr) const;
  int32_t disasm();

  // return a reference to the last instructions operand
  int32_t &get_fixup();

  const uint8_t *get_code() const {
    return code_.data();
  }

  // reset any stored state
  void reset();

protected:
  // write 8bits to the code stream
  void write8(uint8_t v);

  // write 32bits to the code stream
  void write32(int32_t v);

  // add the current code position to the line table usign the line from the
  // given token 't' or the current stream stored line.
  void add_to_linetable(const token_t *t);

  std::map<const uint8_t *, uint32_t> line_table_;

  ccml_t &ccml_;
  uint32_t head_;
  std::array<uint8_t, 1024> code_;
};
