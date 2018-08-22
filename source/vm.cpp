#include "vm.h"
#include "assembler.h"
#include "parser.h"
#include <cstring>

int32_t vm_t::execute(int32_t tgt_pc, int32_t argc, int32_t *argv) {

  thread_t t;

  // save the target pc (entry point)
  t.pc_ = tgt_pc;
  int32_t &pc = t.pc_;

  // get the instruction strea
  const uint8_t *c = ccml_.assembler().get_code();

  // push any arguments
  for (int32_t i = 0; i < argc; ++i) {
    t.push(argv[i]);
  }

  // push the initial frame
  t.new_frame(pc);

  // while we havent returned from frame 0
  while (t.f_.size() > 0) {

#if 1
    // print an instruction trace
    printf(" > ");
    ccml_.assembler().disasm(c + pc);
#endif

    const uint8_t op = c[pc];
    pc += 1;

    switch (op) {
    case (INS_ADD): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l + r);
    }
      continue;
    case (INS_SUB): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l - r);
    }
      continue;
    case (INS_MUL): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l * r);
    }
      continue;
    case (INS_DIV): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l / r);
    }
      continue;
    case (INS_MOD): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l % r);
    }
      continue;
    case (INS_AND): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l && r);
    }
      continue;
    case (INS_OR): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l || r);
    }
      continue;
    case (INS_LT): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l < r);
    }
      continue;
    case (INS_GT): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l > r);
    }
      continue;
    case (INS_LEQ): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l <= r);
    }
      continue;
    case (INS_GEQ): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l >= r);
    }
      continue;
    case (INS_EQ): {
      const int32_t r = t.pop(), l = t.pop();
      t.push(l == r);
    }
      continue;
    case (INS_NOT):
      t.push(!t.pop());
      continue;
    case (INS_NOP):
      continue;
    }

    if (op == INS_SCALL) {
      ccml_syscall_t func = nullptr;
      memcpy(&func, c + pc, sizeof(void *));
      pc += sizeof(void *);
      func(t);
      continue;
    }

    int32_t val = *(int32_t *)(c + pc);
    pc += 4;

    switch (op) {
    case (INS_JMP):
      if (t.pop())
        pc = val;
      break;
    case (INS_CALL):
      // XXX: reserve space for local variables
      t.new_frame(pc);
      pc = val;
      break;
    case (INS_RET):
      pc = t.ret(val);
      break;
    case (INS_POP):
      while (val > 0) {
        t.pop();
        --val;
      };
      break;
    case (INS_CONST):
      t.push(val);
      break;
    case (INS_GETV):
      t.push(t.getv(val));
      break;
    case (INS_SETV):
      t.setv(val, t.pop());
      break;
    case (INS_LOCALS):
      while (val--) {
        t.push(0);
      }
    }
  }

  assert(t.s_.size() > 0);
  return t.s_.back();
}
