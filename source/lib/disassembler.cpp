#include "disassembler.h"
#include "assembler.h"
#include "lexer.h"
#include "instructions.h"


namespace {

const char *gMnemonic[] = {
    // operators
    "INS_ADD", "INS_INC", "INS_SUB", "INS_MUL", "INS_DIV", "INS_MOD", "INS_AND",
    "INS_OR",
    // unary operators
    "INS_NOT", "INS_NEG",
    // comparators
    "INS_LT", "INS_GT", "INS_LEQ", "INS_GEQ", "INS_EQ",
    // branching
    "INS_JMP", "INS_CJMP", "INS_CALL", "INS_RET", "INS_SCALL",
    // stack
    "INS_POP", "INS_CONST", "INS_LOCALS",
    // local variables
    "INS_GETV", "INS_SETV", "INS_GETVI", "INS_SETVI",
    // global variables
    "INS_GETG", "INS_SETG", "INS_GETGI", "INS_SETGI"};

// make sure this is kept up to date with the opcode table 'instruction_e'
static_assert(sizeof(gMnemonic) / sizeof(const char *) == ccml::__INS_COUNT__,
              "gMnemonic table should match instruction_e enum layout");

} // namespace {}

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
    printf("%s\n", gMnemonic[op]);
    return i;
  }

  // syscall
  if (op == INS_SCALL) {
    void *call = 0;
    memcpy(&call, ptr + i, sizeof(void *));
    i += sizeof(void *);
    printf("%-12s %p\n", "INS_SCALL", call);
    return i;
  }

  // instruction with integer operand
  const int32_t val = *(int32_t *)(ptr + i);
  i += 4;

  switch (op) {
  case INS_JMP:
  case INS_CJMP:
  case INS_CALL:
  case INS_RET:
  case INS_POP:
  case INS_CONST:
  case INS_GETV:
  case INS_SETV:
  case INS_LOCALS:
  case INS_GETG:
  case INS_SETG:
  case INS_GETGI:
  case INS_SETGI:
  case INS_GETVI:
  case INS_SETVI:
    printf("%-12s %d\n", gMnemonic[op], val);
    return i;
  }

  return 0;
}

int32_t disassembler_t::disasm() {
  uint32_t count = 0;
  int32_t line_no = -1, old_line = -1;

  // get the code stream
  asm_stream_t &stream = ccml_.store_.stream();

  const uint8_t *start = stream.data();
  const uint8_t *p = stream.data();
  const uint8_t *end = stream.head(0);

  for (; p < end; ++count) {

    const uint8_t *ptr = p;
    bool print_line = false;

    // dump line table mapping
    line_no = ccml_.store_.get_line(p - start);

    if (line_no >= 0 && line_no != old_line) {
      const std::string &line = ccml_.lexer().get_line(line_no);
      printf("  %02d -- %s\n", line_no, line.c_str());
      old_line = line_no;
    }

    printf("%04d ", int32_t(p - start));
    const int32_t nb = disasm(p);
    if (nb <= 0) {
      assert(!"unknown opcode");
    }
    p += nb;
  }
  return count;
}

}