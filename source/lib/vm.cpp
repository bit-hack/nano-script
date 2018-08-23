#include "vm.h"
#include "assembler.h"
#include "parser.h"
#include <cstring>

int32_t vm_t::execute(int32_t tgt_pc, int32_t argc, const int32_t *argv, bool trace) {

  thread_t t;

  // push globals
  for (const auto global : ccml_.parser().globals()) {
    t.g_.push_back(global.value_);
  }

  // save the target pc (entry point)
  t.pc_ = tgt_pc;
  int32_t &pc = t.pc_;

  // get the instruction stream
  const uint8_t *c = ccml_.assembler().get_code();

  // push any arguments
  for (int32_t i = 0; i < argc; ++i) {
    t.push(argv[i]);
  }

  // push the initial frame
  t.new_frame(pc);

  // while we haven't returned from frame 0
  while (t.f_.size() > 0) {

    if (trace) {
      // print an instruction trace
      printf(" > ");
      ccml_.assembler().disasm(c + pc);
    }

    // get opcode
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

    // dispatch system call
    if (op == INS_SCALL) {
      // TODO: use system call table to support x64
      ccml_syscall_t func = nullptr;
      memcpy(&func, c + pc, sizeof(void *));
      pc += sizeof(void *);
      func(t);
      continue;
    }

    // get operand
    const int32_t val = *(int32_t *)(c + pc);
    pc += 4;

    switch (op) {
    case (INS_JMP):
      if (t.pop())
        pc = val;
      continue;
    case (INS_CALL):
      t.new_frame(pc);
      pc = val;
      continue;
    case (INS_RET):
      pc = t.ret(val);
      continue;
    case (INS_POP):
      for (int i = 0; i < val; ++i) {
        t.pop();
      };
      continue;
    case (INS_CONST):
      t.push(val);
      continue;
    case (INS_GETV):
      t.push(t.getv(val));
      continue;
    case (INS_SETV):
      t.setv(val, t.pop());
      continue;
    case (INS_LOCALS):
      // reserve this many values on the stack
      for (int i = 0; i < val; ++i) {
        t.push(0);
      }
      continue;
    case INS_GETG:
      t.push(t.g_[val]);
      continue;
    case INS_SETG:
      t.g_[val] = t.pop();
      continue;
    }

    throws("unknown instruction opcode");

  } // while

  assert(t.s_.size() > 0);
  return t.s_.back();
}
