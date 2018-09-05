#include <cstring>

#include "vm.h"
#include "assembler.h"
#include "parser.h"
#include "disassembler.h"


using namespace ccml;

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

static int32_t history[__INS_COUNT__];

void print_history() {
  printf("histogram:\n");
  for (int32_t i = 0; i < __INS_COUNT__; ++i) {
    const char *name = disassembler_t::get_mnemonic((instruction_e)i);
    printf("  %12s %d\n", name, history[i]);
  }
}

int32_t thread_t::_read_operand() {
  const uint8_t *c = ccml_.code();
  // XXX: range check for c
  const int32_t val = *(int32_t *)(c + pc_);
  pc_ += sizeof(int32_t);
  return val;
}

uint8_t thread_t::_read_opcode() {
  const uint8_t *c = ccml_.code();
  // XXX: range check for c
  const int32_t val = *(uint8_t *)(c + pc_);
  pc_ += sizeof(uint8_t);
  return val;
}

void thread_t::_do_INS_ADD() {
  const value_t r = pop(), l = pop();
  push(l + r);
}

void thread_t::_do_INS_INC() {
  __debugbreak();
  const value_t v = pop();
  const int32_t operand = _read_operand();
  push(v + operand);
}

void thread_t::_do_INS_SUB() {
  const value_t r = pop(), l = pop();
  push(l - r);
}

void thread_t::_do_INS_MUL() {
  const value_t r = pop(), l = pop();
  push(l * r);
}

void thread_t::_do_INS_DIV() {
  const value_t r = pop(), l = pop();
  if (r == 0) {
    set_error(thread_error_t::e_bad_divide_by_zero);
  }
  else {
    push(l / r);
  }
}

void thread_t::_do_INS_MOD() {
  const value_t r = pop(), l = pop();
  push(l % r);
}

void thread_t::_do_INS_AND() {
  const value_t r = pop(), l = pop();
  push(l && r);
}

void thread_t::_do_INS_OR() {
  const value_t r = pop(), l = pop();
  push(l || r);
}

void thread_t::_do_INS_NOT() {
  push(!pop());
}

void thread_t::_do_INS_LT() {
  const value_t r = pop(), l = pop();
  push(l < r);
}

void thread_t::_do_INS_GT() {
  const value_t r = pop(), l = pop();
  push(l > r);
}

void thread_t::_do_INS_LEQ() {
  const value_t r = pop(), l = pop();
  push(l <= r);
}

void thread_t::_do_INS_GEQ() {
  const value_t r = pop(), l = pop();
  push(l >= r);
}

void thread_t::_do_INS_EQ() {
  const value_t r = pop(), l = pop();
  push(l == r);
}

void thread_t::_do_INS_JMP() {
  pc_ = _read_operand();
}

void thread_t::_do_INS_CJMP() {
  const int32_t operand = _read_operand();
  if (!pop()) {
    pc_ = operand;
  }
}

void thread_t::_do_INS_CALL() {
  const int32_t operand = _read_operand();
  // new frame
  enter_(s_head_, pc_);
  pc_ = operand;
}

void thread_t::_do_INS_RET() {
  const int32_t operand = _read_operand();
  // pop return value
  const int32_t sval = pop();
  // remove arguments and local vars
  if (s_head_ - operand < 0) {
    set_error(thread_error_t::e_stack_underflow);
  }
  else {
    s_head_ -= operand;
    // push return value
    push(sval);
    // return to previous frame
    pc_ = leave_();
  }
}

void thread_t::_do_INS_SCALL() {
  const int32_t operand = _read_operand();
  if (const function_t *func = ccml_.parser().find_function(operand)) {
    func->sys_(*this);
  }
  else
  {
    set_error(thread_error_t::e_bad_syscall);
  }
}

void thread_t::_do_INS_POP() {
  const int32_t operand = _read_operand();
  for (int32_t i = 0; i < operand; ++i) {
    pop();
  };
}

void thread_t::_do_INS_CONST() {
  const int32_t operand = _read_operand();
  push(operand);
}

void thread_t::_do_INS_LOCALS() {
  const int32_t operand = _read_operand();
  // reserve this many values on the stack
  if (operand) {
    if (s_head_ + operand >= s_.size()) {
      set_error(thread_error_t::e_stack_overflow);
      return;
    }
    for (int i = 0; i < operand; ++i) {
      s_[s_head_ + i] = 0;
    }
    s_head_ += operand;
  }
}

void thread_t::_do_INS_GETV() {
  const int32_t operand = _read_operand();
  push(getv(operand));
}

void thread_t::_do_INS_SETV() {
  const int32_t operand = _read_operand();
  setv(operand, pop());
}

void thread_t::_do_INS_GETVI() {
  const int32_t operand = _read_operand();
  push(getv(operand + pop()));
}

void thread_t::_do_INS_SETVI() {
  const int32_t operand = _read_operand();
  const int32_t value = pop();
  setv(operand + pop(), value);
}

void thread_t::_do_INS_GETG() {
  const int32_t operand = _read_operand();
  if (operand < 0 || operand >= int32_t(s_.size())) {
    set_error(thread_error_t::e_bad_get_global);
  }
  else {
    push(s_[operand]);
  }
}

void thread_t::_do_INS_SETG() {
  const int32_t operand = _read_operand();
  if (operand < 0 || operand >= int32_t(s_.size())) {
    set_error(thread_error_t::e_bad_set_global);
  }
  else {
    s_[operand] = pop();
  }
}

void thread_t::_do_INS_GETGI() {
  const int32_t operand = _read_operand();
  const int32_t index = operand + pop();
  if (index < 0 || index >= int32_t(s_.size())) {
    set_error(thread_error_t::e_bad_get_global);
  } else {
    push(s_[index]);
  }
}

void thread_t::_do_INS_SETGI() {
  const int32_t operand = _read_operand();
  const value_t value = pop();
  const int32_t index = operand + pop();
  if (index < 0 || index >= int32_t(s_.size())) {
    set_error(thread_error_t::e_bad_set_global);
  } else {
    s_[index] = value;
  }
}

bool thread_t::prepare(const function_t &func, int32_t argc, const value_t *argv) {

  error_ = thread_error_t::e_success;
  finished_ = true;
  cycles_ = 0;

  // push globals
  if (const int32_t size = ccml_.parser().global_size()) {
    if (size < 0 || size >= int32_t(s_.size())) {
      set_error(thread_error_t::e_bad_globals_size);
      return false;
    }
    // reserve this much space for globals
    s_head_ += size;
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
    error_ = thread_error_t::e_bad_num_args;
    return false;
  }

  // push any arguments
  for (value_t i = 0; i < argc; ++i) {
    push(argv[i]);
  }

  // push the initial frame
  enter_(s_head_, pc_);

  // catch any misc errors
  if (error_ != thread_error_t::e_success) {
    return false;
  }

  // good to go
  finished_ = false;
  return true;
}

bool thread_t::resume(int32_t cycles, bool trace) {
  if (finished_) {
    return false;
  }
  const uint32_t start_cycles = cycles;
  // while we should keep processing instructions
  while (cycles > 0 && f_head_) {
    // fetch opcode
    const uint8_t opcode = _read_opcode();
    ++history[opcode];
    // print an instruction trace
    if (trace) {
      const uint8_t *c = ccml_.code();
      printf(" > ");
      ccml_.disassembler().disasm(c + pc_);
    }
    // dispatch
    switch (opcode) {
    case INS_ADD:    _do_INS_ADD();    break;
    case INS_INC:    _do_INS_INC();    break;
    case INS_SUB:    _do_INS_SUB();    break;
    case INS_MUL:    _do_INS_MUL();    break;
    case INS_DIV:    _do_INS_DIV();    break;
    case INS_MOD:    _do_INS_MOD();    break;
    case INS_AND:    _do_INS_AND();    break;
    case INS_OR:     _do_INS_OR();     break;
    case INS_NOT:    _do_INS_NOT();    break;
    case INS_LT:     _do_INS_LT();     break;
    case INS_GT:     _do_INS_GT();     break;
    case INS_LEQ:    _do_INS_LEQ();    break;
    case INS_GEQ:    _do_INS_GEQ();    break;
    case INS_EQ:     _do_INS_EQ();     break;
    case INS_JMP:    _do_INS_JMP();    break;
    case INS_CJMP:   _do_INS_CJMP();   break;
    case INS_CALL:   _do_INS_CALL();   break;
    case INS_RET:    _do_INS_RET();    break;
    case INS_SCALL:  _do_INS_SCALL();  break;
    case INS_POP:    _do_INS_POP();    break;
    case INS_CONST:  _do_INS_CONST();  break;
    case INS_LOCALS: _do_INS_LOCALS(); break;
    case INS_GETV:   _do_INS_GETV();   break;
    case INS_SETV:   _do_INS_SETV();   break;
    case INS_GETVI:  _do_INS_GETVI();  break;
    case INS_SETVI:  _do_INS_SETVI();  break;
    case INS_GETG:   _do_INS_GETG();   break;
    case INS_SETG:   _do_INS_SETG();   break;
    case INS_GETGI:  _do_INS_GETGI();  break;
    case INS_SETGI:  _do_INS_SETGI();  break;
    default:
      set_error(thread_error_t::e_bad_opcode);
      break;
    }
    --cycles;
    if (has_error() || finished_) {
      break;
    }
  }
  // check for program termination
  if (!finished_ && !f_head_) {
    assert(s_head_ > 0);
    return_code_ = pop_();
    finished_ = true;
  }
  // increment the cycle count
  cycles_ += (start_cycles - cycles);
  return !has_error();
}

bool vm_t::execute(const function_t &func, int32_t argc, const value_t *argv,
                   int32_t *ret, bool trace) {
  thread_t t{ccml_};
  if (!t.prepare(func, argc, argv)) {
    return false;
  }
  if (!t.resume(INT_MAX, trace)) {
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

value_t thread_t::getv(int32_t offs) {
  const int32_t index = frame_().sp_ + offs;
  if (index < 0 || index >= int32_t(s_head_)) {
    set_error(thread_error_t::e_bad_getv);
    return 0;
  }
  return s_[index];
}

void thread_t::setv(int32_t offs, value_t val) {
  const int32_t index = frame_().sp_ + offs;
  if (index < 0 || index >= int32_t(s_head_)) {
    set_error(thread_error_t::e_bad_setv);
  }
  else {
    s_[index] = val;
  }
}
