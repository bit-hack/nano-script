#pragma once
#include <cstring>
#include <string>
#include <array>
#include <map>

#include "ccml.h"
#include "token.h"


namespace ccml {

enum instruction_e {
  INS_ADD,
  INS_INC,
  INS_SUB,
  INS_MUL,
  INS_DIV,
  INS_MOD,
  INS_AND,
  INS_OR,
  INS_NOT,

  // compare less than
  //    push( pop() < pop() )
  INS_LT,

  // compare greater then
  INS_GT,
  INS_LEQ,
  INS_GEQ,
  INS_EQ,

  // unconditional jump
  //    pc = operand
  INS_JMP,

  // conditional jump to code offset
  //    if (pop() != 0)
  //      pc = operand
  // note: this would be more efficient in the general case to branch when the
  //       condition == 0.  we can avoid a INS_NOT instruction.
  INS_CJMP,

  // call a function
  INS_CALL,

  // return to previous frame {popping locals and args}
  INS_RET,

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

  // number of instructions
  __INS_COUNT__,
};

struct asm_stream_t {

  asm_stream_t(uint8_t *base, size_t size)
    : end(base + size)
    , start(base)
    , ptr(base)
  {
  }

  bool write1(const uint8_t data) {
    if (end - ptr > 1) {
      *ptr = data;
      ptr += 1;
      return true;
    }
    return false;
  }

  bool write4(const uint32_t data) {
    if (end - ptr > 4) {
      memcpy(ptr, &data, 4);
      ptr += 4;
      return true;
    }
    return false;
  }

  const uint8_t *tail() const {
    return end;
  }

  size_t written() const {
    return ptr - start;
  }

  const uint8_t *data() const {
    return start;
  }

  size_t size() const {
    return end - start;
  }

  uint32_t pos() const {
    return ptr - start;
  }

  uint8_t *head(int32_t adjust) const {
    uint8_t *p = ptr + adjust;
    assert(p < end && p >= start);
    return p;
  }

protected:
  const uint8_t *end;
  uint8_t *start;
  uint8_t *ptr;
};

struct assembler_t {

  assembler_t(ccml_t &c);

  // emit into the code stream
  void     emit(token_e, const token_t *t = nullptr);
  void     emit(instruction_e ins, const token_t *t = nullptr);
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

  // get instruction mnemonic
  static const char *get_mnemonic(instruction_e e);

  // set the assembler output stream
  // XXX: note, changing output streams will also mess up the line table :(
  void set_stream(asm_stream_t *asm_stream) {
    stream_ = asm_stream;
  }

protected:
  // write 8bits to the code stream
  void write8(const uint8_t v);

  // write 32bits to the code stream
  void write32(const int32_t v);

  // add the current code position to the line table usign the line from the
  // given token 't' or the current stream stored line.
  void add_to_linetable(const token_t *t);

  std::map<const uint8_t *, uint32_t> line_table_;

  ccml_t &ccml_;

  // the currently bound code stream
  asm_stream_t *stream_;

  // the default code stream
  asm_stream_t code_stream_;
  std::array<uint8_t, 1024 * 8> code_;
};

} // namespace ccml
