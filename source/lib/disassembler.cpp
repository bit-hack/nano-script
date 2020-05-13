#include "disassembler.h"
#include "codegen.h"
#include "lexer.h"
#include "instructions.h"


namespace {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
const char *gMnemonic[] = {
  // operators
  "INS_ADD", "INS_SUB", "INS_MUL", "INS_DIV", "INS_MOD", "INS_AND", "INS_OR",
  // unary operators
  "INS_NOT", "INS_NEG",
  // comparators
  "INS_LT", "INS_GT", "INS_LEQ", "INS_GEQ", "INS_EQ",
  // branching
  "INS_JMP", "INS_TJMP", "INS_FJMP", "INS_CALL", "INS_RET", "INS_SCALL",
  "INS_ICALL",
  // stack
  "INS_POP",
  "INS_NEW_INT", "INS_NEW_STR", "INS_NEW_ARY", "INS_NEW_NONE", "INS_NEW_FLT",
  "INS_NEW_FUNC", "INS_NEW_SYSCALL", "INS_LOCALS", "INS_GLOBALS",
  // local variables
  "INS_GETV", "INS_SETV", "INS_GETA", "INS_SETA",
  // global variables
  "INS_GETG", "INS_SETG"
};

// make sure this is kept up to date with the opcode table 'instruction_e'
static_assert(sizeof(gMnemonic) / sizeof(const char *) == ccml::__INS_COUNT__,
              "gMnemonic table should match instruction_e enum layout");

} // namespace {}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
namespace ccml {

const char *disassembler_t::get_mnemonic(const instruction_e e) {
  assert(e < __INS_COUNT__);
  return gMnemonic[e];
}

// disassemble a code stream
int32_t disassembler_t::disasm(const uint8_t *ptr) const {
    // extract opcode
  uint32_t i = 0;
  const uint8_t op = ptr[i];
  i += 1;

  // instructions with no operand
  switch (op) {
  case INS_ADD:
  case INS_SUB:
  case INS_MUL:
  case INS_DIV:
  case INS_MOD:
  case INS_AND:
  case INS_OR:
  case INS_NOT:
  case INS_NEG:
  case INS_LT:
  case INS_GT:
  case INS_LEQ:
  case INS_GEQ:
  case INS_EQ:
  case INS_SETA:
  case INS_GETA:
  case INS_NEW_NONE:
    fprintf(fd_, "%s\n", gMnemonic[op]);
    return i;
  }

  // instruction with integer operand
  const int32_t val1 = *(int32_t *)(ptr + i);
  i += 4;

  switch (op) {
  case INS_JMP:
  case INS_TJMP:
  case INS_FJMP:
  case INS_RET:
  case INS_POP:
  case INS_NEW_ARY:
  case INS_NEW_INT:
  case INS_NEW_STR:
  case INS_NEW_FLT:
  case INS_NEW_FUNC:
  case INS_NEW_SCALL:
  case INS_GETV:
  case INS_SETV:
  case INS_LOCALS:
  case INS_GLOBALS:
  case INS_GETG:
  case INS_SETG:
  case INS_ICALL:
    fprintf(fd_, "%-12s %d\n", gMnemonic[op], val1);
    return i;
  }

  // instruction with second integer operand
  const int32_t val2 = *(int32_t *)(ptr + i);
  i += 4;

  switch (op) {
  case INS_SCALL:
  case INS_CALL:
    fprintf(fd_, "%-12s %d %d\n", gMnemonic[op], val1, val2);
    return i;
  }

  return 0;
}

bool disassembler_t::disasm(program_t &prog, int32_t &index, instruction_t &out) const {

  const uint8_t *start = prog.data();
  const uint8_t *p     = prog.data() + index;

  if (p < start || p >= prog.end()) {
    return false;
  }

  out.token = nullptr;
  out.operand = 0;

  out.opcode = (instruction_e)(p[0]);
  index += 1;
  if (ins_has_operand(out.opcode)) {
    out.operand = *(int32_t*)(p + 1);
    index += 4;
  }

  return true;
}

} // namespace ccml
