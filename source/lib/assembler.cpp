#include <cstring>
#include "assembler.h"

namespace {

const char *gMnemonic[] = {
    "INS_ADD", "INS_SUB",   "INS_MUL",    "INS_DIV",  "INS_MOD",
    "INS_AND", "INS_OR",    "INS_NOT",    "INS_LT",   "INS_GT",
    "INS_LEQ", "INS_GEQ",   "INS_EQ",     "INS_JMP",  "INS_CALL",
    "INS_RET", "INS_POP",   "INS_CONST",  "INS_GETV", "INS_SETV",
    "INS_NOP", "INS_SCALL", "INS_LOCALS", "INS_GETG", "INS_SETG"};

// make sure this is kept up to date with the opcode table 'instruction_e'
static_assert(sizeof(gMnemonic) / sizeof(const char*) == __INS_COUNT__, "");
};

assembler_t::assembler_t(ccml_t &c)
  : ccml_(c)
  , head_(0)
{}

void assembler_t::write8(uint8_t v) {
  code_[head_++] = v;
}

void assembler_t::write32(int32_t v) {
  memcpy(code_.data() + head_, &v, sizeof(v));
  head_ += 4;
}

void assembler_t::emit(uint32_t line, instruction_e ins) {
  // insert into the line table
  const uint8_t *ptr = code_.data() + head_;
  line_table_[ptr] = line;
  // encode this instruction
  switch (ins) {
  case INS_ADD:
  case INS_SUB:
  case INS_MUL:
  case INS_DIV:
  case INS_MOD:
  case INS_AND:
  case INS_OR:
  case INS_NOT:
  case INS_LT:
  case INS_GT:
  case INS_LEQ:
  case INS_GEQ:
  case INS_EQ:
  case INS_NOP:
    write8(ins);
    break;
  default:
    throws("unknown instruction");
  }
}

int32_t *assembler_t::emit(uint32_t line, instruction_e ins, int32_t v) {
  // insert into the line table
  const uint8_t *ptr = code_.data() + head_;
  line_table_[ptr] = line;
  // encode this instruction
  switch (ins) {
  case INS_JMP:
  case INS_CALL:
  case INS_RET:
  case INS_POP:
  case INS_CONST:
  case INS_GETV:
  case INS_SETV:
  case INS_LOCALS:
  case INS_GETG:
  case INS_SETG:
    write8(ins);
    write32(v);
    break;
  default:
    throws("unknown instruction");
  }
  // return the operand
  return (int32_t *)(code_.data() + (head_ - 4));
}

void assembler_t::emit(uint32_t line, ccml_syscall_t sys) {
  write8(INS_SCALL);
  memcpy(code_.data() + head_, &sys, sizeof(ccml_syscall_t));
  head_ += sizeof(ccml_syscall_t);
}

int32_t assembler_t::pos() const {
  return head_;
}

// return number of bytes disassembled or <= 0 on error
int32_t assembler_t::disasm(const uint8_t *ptr) const {

  // dump line table mapping
  auto itt = line_table_.find(ptr);
  if (itt != line_table_.end()) {
    printf("%02d  ", itt->second);
  }

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
  case INS_LT:
  case INS_GT:
  case INS_LEQ:
  case INS_GEQ:
  case INS_EQ:
  case INS_NOP:
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
  case INS_CALL:
  case INS_RET:
  case INS_POP:
  case INS_CONST:
  case INS_GETV:
  case INS_SETV:
  case INS_LOCALS:
  case INS_GETG:
  case INS_SETG:
    printf("%-12s %d\n", gMnemonic[op], val);
    return i;
  }

  return 0;
}

int32_t assembler_t::disasm() {
  uint32_t count = 0;
  for (uint32_t i = 0; i < head_; ++count) {
    printf("%04d ", i);
    const int32_t nb = disasm(code_.data() + i);
    if (nb <= 0) {
      throws("unknown opcode");
    }
    i += nb;
  }
  return count;
}

void assembler_t::emit(uint32_t line, token_e tok) {
  switch (tok) {
  case TOK_ADD: emit(line, INS_ADD); break;
  case TOK_SUB: emit(line, INS_SUB); break;
  case TOK_MUL: emit(line, INS_MUL); break;
  case TOK_DIV: emit(line, INS_DIV); break;
  case TOK_MOD: emit(line, INS_MOD); break;
  case TOK_AND: emit(line, INS_AND); break;
  case TOK_OR:  emit(line, INS_OR ); break;
  case TOK_NOT: emit(line, INS_NOT); break;
  case TOK_EQ:  emit(line, INS_EQ ); break;
  case TOK_LT:  emit(line, INS_LT ); break;
  case TOK_GT:  emit(line, INS_GT ); break;
  case TOK_LEQ: emit(line, INS_LEQ); break;
  case TOK_GEQ: emit(line, INS_GEQ); break;
  default:
    throws("cant emit token type");
  }
}

int32_t &assembler_t::get_fixup() {
  // warning: if code_ can grow this will error
  assert(head_ >= sizeof(int32_t));
  return *reinterpret_cast<int32_t*>(code_.data() + (head_ - sizeof(int32_t)));
}
