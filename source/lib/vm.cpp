#include <cstring>

#include "vm.h"
#include "assembler.h"
#include "parser.h"

/*
 *   s_     STACK LAYOUT
 *        .  .  .  .  .  .
 *        |              |
 *   n    |var 0         |  <-- Frame 0 Pointer
 *   n-1  |arg 0         |
 *   n-2  |var 1         |
 *   n-3  |var 0         |  <-- Frame 1 Pointer
 *   n-4  |arg 1         |
 *   n-5  |arg 0         |
 *   n-6  |var 2         |
 *   n-7  |var 1         |
 *   n-8  |var 0         |  <-- Frame 2 Pointer
 *        |              |
 *        |....          |  ...
 *        |              |
 *   1    |global 1      |
 *   0    |global 0      |
 *        '--------------'
 */

bool thread_t::prepare(const function_t &func, int32_t argc, const int32_t *argv) {

  finished_ = true;
  cycles_ = 0;

  // push globals
  if (const int32_t size = ccml_.parser().global_size()) {
    s_.resize(size);
    for (const auto &g : ccml_.parser().globals()) {
      if (g.size_ == 1) {
        s_[g.offset_] = g.value_;
      }
    }
  }

  // save the target pc (entry point)
  pc_ = func.pos_;
  if (func.num_args_ != argc) {
    return_code_ = -1;
    error_ = "incorrect number of arguments";
    return false;
  }

  // push any arguments
  for (int32_t i = 0; i < argc; ++i) {
    push(argv[i]);
  }

  // push the initial frame
  new_frame(pc_);

  // good to go
  error_.clear();
  finished_ = false;
  return true;
}

bool thread_t::resume(uint32_t cycles, bool trace) {

  if (finished_) {
    return false;
  }

  // get the instruction stream
  const uint8_t *c = ccml_.assembler().get_code();

  // while we haven't returned from frame 0
  while (--cycles && f_.size()) {

    // increment the cycle count
    ++cycles_;

    if (trace) {
      // print an instruction trace
      printf(" > ");
      ccml_.assembler().disasm(c + pc_);
    }

    // get opcode
    const uint8_t op = c[pc_];
    pc_ += 1;

    // dispatch instructions that don't require an operand
#define OPERATOR(OP)                                                           \
  {                                                                            \
    const int32_t r = pop(), l = pop();                                        \
    push(l OP r);                                                              \
  }
    switch (op) {
    case (INS_ADD): OPERATOR(+ ); continue;
    case (INS_SUB): OPERATOR(- ); continue;
    case (INS_MUL): OPERATOR(* ); continue;
    case (INS_DIV): OPERATOR(/ ); continue;
    case (INS_MOD): OPERATOR(% ); continue;
    case (INS_AND): OPERATOR(&&); continue;
    case (INS_OR):  OPERATOR(||); continue;
    case (INS_LT):  OPERATOR(< ); continue;
    case (INS_GT):  OPERATOR(> ); continue;
    case (INS_LEQ): OPERATOR(<=); continue;
    case (INS_GEQ): OPERATOR(>=); continue;
    case (INS_EQ):  OPERATOR(==); continue;
    case (INS_NOT): push(!pop()); continue;
      continue;
    }
#undef OPERATOR

    // get operand
    const int32_t val = *(int32_t *)(c + pc_);
    pc_ += 4;

    // dispatch instructions that require one operand
    switch (op) {
    case INS_SCALL:
      if (const function_t *func = ccml_.parser().find_function(val)) {
        func->sys_(*this);
        continue;
      }
      // exception
      {
        finished_ = true;
        error_ = "Unknown system call";
        return_code_ = -1;
        return false;
      }
    case INS_JMP:
      pc_ = val;
      continue;
    case INS_CJMP:
      if (pop()) {
        pc_ = val;
      }
      continue;
    case INS_CALL:
      new_frame(pc_);
      pc_ = val;
      continue;
    case INS_RET:
      pc_ = ret(val);
      continue;
    case INS_POP:
      for (int32_t i = 0; i < val; ++i) {
        pop();
      };
      continue;
    case INS_CONST:
      push(val);
      continue;
    case INS_LOCALS:
      // reserve this many values on the stack
      if (val) {
        s_.resize(s_.size() + val, 0);
      }
      continue;
    case INS_GETV:
      push(getv(val));
      continue;
    case INS_SETV:
      setv(val, pop());
      continue;
    case INS_GETVI:
      push(getv(val + pop()));
      continue;
    case INS_GETGI:
      push(s_[val + pop()]);
      continue;
    case INS_GETG:
      push(s_[val]);
      continue;
    case INS_SETG:
      s_[val] = pop();
      continue;
    }

    // dispatch instructions that require more operands
    if (op == INS_SETVI) {
      const int32_t value = pop();
      setv(val + pop(), value);
      continue;
    }
    if (op == INS_SETGI) {
      const int32_t value = pop();
      s_[val + pop()] = value;
      continue;
    }

    // exception
    {
      finished_ = true;
      error_ = "unknown instruction opcode";
      return_code_ = -1;
      return false;
    }
  } // while

  // check for program termination
  if (!finished_ && f_.empty()) {
    assert(s_.size() > 0);
    return_code_ = s_.back();
    finished_ = true;
  }

  return true;
}

bool vm_t::execute(const function_t &func, int32_t argc, const int32_t *argv, int32_t *ret, bool trace) {
  thread_t t{ccml_};
  if (!t.prepare(func, argc, argv)) {
    return false;
  }
  if (!t.resume(-1, trace)) {
    return false;
  }
  if (!t.finished()) {
    return false;
  }
  if (ret) {
    *ret = t.return_code();
  }
  return true;
}

void vm_t::reset() {
  // nothing to do
}

int32_t thread_t::getv(int32_t offs) {
  const int32_t index = f_.back().sp_ + offs;
  if (index < 0 || index >= int32_t(s_.size())) {
    ccml_.assembler().disasm();
    __debugbreak();
  }
  return s_[index];
}

void thread_t::setv(int32_t offs, int32_t val) {
  const int32_t index = f_.back().sp_ + offs;
  if (index < 0 || index >= int32_t(s_.size())) {
    ccml_.assembler().disasm();
    __debugbreak();
  }
  s_[index] = val;
}
