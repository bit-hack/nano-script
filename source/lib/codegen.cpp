#include <cstring>

#include "assembler.h"
#include "disassembler.h"
#include "lexer.h"
#include "ccml.h"
#include "errors.h"
#include "instructions.h"
#include "parser.h"


using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void asm_stream_t::set_line(lexer_t &lexer, const token_t *t) {
  const uint32_t pc = pos();
  const uint32_t line = t ? t->line_no_ : lexer.stream_.line_number();
  store_.add_line(pc, line);
}

void asm_stream_t::add_ident(const identifier_t &ident) {
  for (auto &i : store_.identifiers_) {
    if (i.token == ident.token) {
      i.start = std::min(ident.start, i.start);
      i.end   = std::max(ident.end,   i.end);
      return;
    }
  }
  store_.identifiers_.push_back(ident);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
assembler_t::assembler_t(ccml_t &c, asm_stream_t &stream)
  : ccml_(c)
  , stream_(stream)
{
}

void assembler_t::emit(instruction_e ins, const token_t *t) {
  stream_.set_line(ccml_.lexer(), t);
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
  case INS_NEG:
    emit_(instruction_t{ins, 0, t});
    break;
  default:
    assert(!"unknown instruction");
  }
}

void assembler_t::emit(instruction_e ins, int32_t v, const token_t *t) {
  stream_.set_line(ccml_.lexer(), t);
  // encode this instruction
  switch (ins) {
  case INS_JMP:
  case INS_FJMP:
  case INS_TJMP:
  case INS_CALL:
  case INS_RET:
  case INS_SCALL:
  case INS_POP:
  case INS_CONST:
  case INS_LOCALS:
  case INS_ACCV:
  case INS_GETV:
  case INS_SETV:
  case INS_GETVI:
  case INS_SETVI:
  case INS_GETG:
  case INS_SETG:
  case INS_GETGI:
  case INS_SETGI:
    emit_(instruction_t{ins, v, t});
    break;
  default:
    assert(!"unknown instruction");
  }
}

void assembler_t::emit(token_e tok, const token_t *t) {
  switch (tok) {
  case TOK_ADD: emit(INS_ADD, t); break;
  case TOK_SUB: emit(INS_SUB, t); break;
  case TOK_MUL: emit(INS_MUL, t); break;
  case TOK_DIV: emit(INS_DIV, t); break;
  case TOK_MOD: emit(INS_MOD, t); break;
  case TOK_AND: emit(INS_AND, t); break;
  case TOK_OR:  emit(INS_OR , t); break;
  case TOK_NOT: emit(INS_NOT, t); break;
  case TOK_NEG: emit(INS_NEG, t); break;
  case TOK_EQ:  emit(INS_EQ , t); break;
  case TOK_LT:  emit(INS_LT , t); break;
  case TOK_GT:  emit(INS_GT , t); break;
  case TOK_LEQ: emit(INS_LEQ, t); break;
  case TOK_GEQ: emit(INS_GEQ, t); break;
  default:
    assert(!"cant emit token type");
  }
}

int32_t assembler_t::pos() {
  return stream_.pos();
}

int32_t *assembler_t::get_fixup() {
  return reinterpret_cast<int32_t*>(stream_.head(-4));
}

void assembler_t::reset() {
}

// emit an instruction into the instruction stream
void assembler_t::emit_(const instruction_t &ins) {
}
