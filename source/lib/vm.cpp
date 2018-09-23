#include <cstring>

#include "vm.h"
#include "codegen.h"
#include "parser.h"
#include "disassembler.h"


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


using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
namespace {

int32_t history[__INS_COUNT__];

} // namespace {}

void print_history() {
  printf("histogram:\n");
  for (int32_t i = 0; i < __INS_COUNT__; ++i) {
    const char *name = disassembler_t::get_mnemonic((instruction_e)i);
    printf("  %12s %d\n", name, history[i]);
  }
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
int32_t thread_t::read_operand_() {
  const uint8_t *c = ccml_.code();
  // XXX: range check for c
  const int32_t val = *(int32_t *)(c + pc_);
  pc_ += sizeof(int32_t);
  return val;
}

uint8_t thread_t::peek_opcode_() {
  const uint8_t *c = ccml_.code();
  // XXX: range check for c
  const int32_t val = *(uint8_t *)(c + pc_);
  return val;
}

uint8_t thread_t::read_opcode_() {
  const uint8_t *c = ccml_.code();
  // XXX: range check for c
  const int32_t val = *(uint8_t *)(c + pc_);
  pc_ += sizeof(uint8_t);
  return val;
}

void thread_t::do_INS_ADD_() {
  const value_t r = pop(), l = pop();
  push(l + r);
}

void thread_t::do_INS_SUB_() {
  const value_t r = pop(), l = pop();
  push(l - r);
}

void thread_t::do_INS_MUL_() {
  const value_t r = pop(), l = pop();
  push(l * r);
}

void thread_t::do_INS_DIV_() {
  const value_t r = pop(), l = pop();
  if (r == 0) {
    set_error_(thread_error_t::e_bad_divide_by_zero);
  }
  else {
    push(l / r);
  }
}

void thread_t::do_INS_MOD_() {
  const value_t r = pop(), l = pop();
  push(l % r);
}

void thread_t::do_INS_AND_() {
  const value_t r = pop(), l = pop();
  push(l && r);
}

void thread_t::do_INS_OR_() {
  const value_t r = pop(), l = pop();
  push(l || r);
}

void thread_t::do_INS_NOT_() {
  push(!pop());
}

void thread_t::do_INS_NEG_() {
  push(-pop());
}

void thread_t::do_INS_LT_() {
  const value_t r = pop(), l = pop();
  push(l < r);
}

void thread_t::do_INS_GT_() {
  const value_t r = pop(), l = pop();
  push(l > r);
}

void thread_t::do_INS_LEQ_() {
  const value_t r = pop(), l = pop();
  push(l <= r);
}

void thread_t::do_INS_GEQ_() {
  const value_t r = pop(), l = pop();
  push(l >= r);
}

void thread_t::do_INS_EQ_() {
  const value_t r = pop(), l = pop();
  push(l == r);
}

void thread_t::do_INS_JMP_() {
  pc_ = read_operand_();
}

void thread_t::do_INS_TJMP_() {
  const int32_t operand = read_operand_();
  if (pop()) {
    pc_ = operand;
  }
}

void thread_t::do_INS_FJMP_() {
  const int32_t operand = read_operand_();
  if (!pop()) {
    pc_ = operand;
  }
}

void thread_t::do_INS_CALL_() {
  const int32_t operand = read_operand_();
  // new frame
  enter_(s_head_, pc_);
  pc_ = operand;
}

void thread_t::do_INS_RET_() {
  const int32_t operand = read_operand_();
  // pop return value
  const int32_t sval = pop();
  // remove arguments and local vars
  if (int32_t(s_head_) < operand) {
    set_error_(thread_error_t::e_stack_underflow);
  }
  else {
    s_head_ -= operand;
    // push return value
    push(sval);
    // return to previous frame
    pc_ = leave_();
  }
}

void thread_t::do_INS_SCALL_() {
  const int32_t operand = read_operand_();

  const auto &funcs = ccml_.functions();
  if (operand < 0 || operand >= funcs.size()) {
    set_error_(thread_error_t::e_bad_syscall);
  }
  else {
    const function_t *func = &ccml_.functions()[operand];
    func->sys_(*this);
  }
}

void thread_t::do_INS_POP_() {
  const int32_t operand = read_operand_();
  for (int32_t i = 0; i < operand; ++i) {
    pop();
  };
}

void thread_t::do_INS_CONST_() {
  const int32_t operand = read_operand_();
  push(operand);
}

void thread_t::do_INS_LOCALS_() {
  const int32_t operand = read_operand_();
  // reserve this many values on the stack
  if (operand) {
    if (s_head_ + operand >= s_.size()) {
      set_error_(thread_error_t::e_stack_overflow);
      return;
    }
    for (int i = 0; i < operand; ++i) {
      s_[s_head_ + i] = 0;
    }
    s_head_ += operand;
  }
}

void thread_t::do_INS_ACCV_() {
  const int32_t operand = read_operand_();
  const value_t val = pop();
  setv_(operand, getv_(operand) + val);
}

void thread_t::do_INS_GETV_() {
  const int32_t operand = read_operand_();
  push(getv_(operand));
}

void thread_t::do_INS_SETV_() {
  const int32_t operand = read_operand_();
  setv_(operand, pop());
}

void thread_t::do_INS_GETVI_() {
  const int32_t operand = read_operand_();
  push(getv_(operand + pop()));
}

void thread_t::do_INS_SETVI_() {
  const int32_t operand = read_operand_();
  const int32_t value = pop();
  setv_(operand + pop(), value);
}

void thread_t::do_INS_GETG_() {
  const int32_t operand = read_operand_();
  if (operand < 0 || operand >= int32_t(s_.size())) {
    set_error_(thread_error_t::e_bad_get_global);
  }
  else {
    push(s_[operand]);
  }
}

void thread_t::do_INS_SETG_() {
  const int32_t operand = read_operand_();
  if (operand < 0 || operand >= int32_t(s_.size())) {
    set_error_(thread_error_t::e_bad_set_global);
  }
  else {
    s_[operand] = pop();
  }
}

void thread_t::do_INS_GETGI_() {
  const int32_t operand = read_operand_();
  const int32_t index = operand + pop();
  if (index < 0 || index >= int32_t(s_.size())) {
    set_error_(thread_error_t::e_bad_get_global);
  } else {
    push(s_[index]);
  }
}

void thread_t::do_INS_SETGI_() {
  const int32_t operand = read_operand_();
  const value_t value = pop();
  const int32_t index = operand + pop();
  if (index < 0 || index >= int32_t(s_.size())) {
    set_error_(thread_error_t::e_bad_set_global);
  } else {
    s_[index] = value;
  }
}

namespace {

int32_t global_space(const std::vector<global_t> &globals) {
  int32_t sum = 0;
  for (const auto &g : globals) {
    sum += g.size_;
  }
  return sum;
}

} // namespace {}

bool thread_t::prepare(const function_t &func, int32_t argc, const value_t *argv) {
  error_ = thread_error_t::e_success;
  finished_ = true;
  cycles_ = 0;
  halted_ = false;
  s_head_ = 0;

  // push globals
  if (!ccml_.globals().empty()) {
    //
    const int32_t size = global_space(ccml_.globals());
    // reserve this much space for globals
    s_head_ += size;
    for (const auto &g : ccml_.globals()) {
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

void thread_t::step_imp_() {
  // fetch opcode
  const uint8_t opcode = read_opcode_();
  // dispatch
  switch (opcode) {
  case INS_ADD:    do_INS_ADD_();    break;
  case INS_SUB:    do_INS_SUB_();    break;
  case INS_MUL:    do_INS_MUL_();    break;
  case INS_DIV:    do_INS_DIV_();    break;
  case INS_MOD:    do_INS_MOD_();    break;
  case INS_AND:    do_INS_AND_();    break;
  case INS_OR:     do_INS_OR_();     break;
  case INS_NOT:    do_INS_NOT_();    break;
  case INS_NEG:    do_INS_NEG_();    break;
  case INS_LT:     do_INS_LT_();     break;
  case INS_GT:     do_INS_GT_();     break;
  case INS_LEQ:    do_INS_LEQ_();    break;
  case INS_GEQ:    do_INS_GEQ_();    break;
  case INS_EQ:     do_INS_EQ_();     break;
  case INS_JMP:    do_INS_JMP_();    break;
  case INS_TJMP:   do_INS_TJMP_();   break;
  case INS_FJMP:   do_INS_FJMP_();   break;
  case INS_CALL:   do_INS_CALL_();   break;
  case INS_RET:    do_INS_RET_();    break;
  case INS_SCALL:  do_INS_SCALL_();  break;
  case INS_POP:    do_INS_POP_();    break;
  case INS_CONST:  do_INS_CONST_();  break;
  case INS_LOCALS: do_INS_LOCALS_(); break;
  case INS_ACCV:   do_INS_ACCV_();   break;
  case INS_GETV:   do_INS_GETV_();   break;
  case INS_SETV:   do_INS_SETV_();   break;
  case INS_GETVI:  do_INS_GETVI_();  break;
  case INS_SETVI:  do_INS_SETVI_();  break;
  case INS_GETG:   do_INS_GETG_();   break;
  case INS_SETG:   do_INS_SETG_();   break;
  case INS_GETGI:  do_INS_GETGI_();  break;
  case INS_SETGI:  do_INS_SETGI_();  break;
  default:
    set_error_(thread_error_t::e_bad_opcode);
  }
}

bool thread_t::step_inst() {
  if (finished_) {
    return false;
  }
  step_imp_();
  // check for program termination
  if (!finished_ && !f_head_) {
    assert(s_head_ > 0);
    return_code_ = pop_();
    finished_ = true;
  }
  return !has_error();
}

bool thread_t::step_line() {
  if (finished_) {
    return false;
  }
  // get the current source line
  const uint32_t line = source_line();
  // step until the source line changes
  do {
    step_imp_();
    if (finished_ || !f_head_ || has_error()) {
      break;
    }
  } while (line == source_line());
  // check for program termination
  if (!finished_ && !f_head_) {
    assert(s_head_ > 0);
    return_code_ = pop_();
    finished_ = true;
  }
  return !has_error();
}

int32_t thread_t::source_line() const {
  return ccml_.store().get_line(pc_);
}

bool thread_t::resume(int32_t cycles, bool trace) {
  if (finished_) {
    return false;
  }
  const uint32_t start_cycles = cycles;
  // while we should keep processing instructions
  while (cycles > 0 && f_head_) {
    --cycles;
    // fetch opcode
    const uint8_t opcode = peek_opcode_();
    ++history[opcode];
    // print an instruction trace
    if (trace) {
      const uint8_t *c = ccml_.code();
      printf(" > ");
      ccml_.disassembler().disasm(c + pc_);
    }
    // dispatch
    step_imp_();
    if (has_error() || finished_ || halted_) {
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

bool thread_t::active_vars(std::vector<const identifier_t *> &out) const {
  return ccml_.store().active_vars(pc_, out);
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

value_t thread_t::getv_(int32_t offs) {
  const int32_t index = frame_().sp_ + offs;
  if (index < 0 || index >= int32_t(s_head_)) {
    set_error_(thread_error_t::e_bad_getv);
    return 0;
  }
  return s_[index];
}

void thread_t::setv_(int32_t offs, value_t val) {
  const int32_t index = frame_().sp_ + offs;
  if (index < 0 || index >= int32_t(s_head_)) {
    set_error_(thread_error_t::e_bad_setv);
  }
  else {
    s_[index] = val;
  }
}

// peek a stack value
bool thread_t::peek(int32_t offset, bool absolute, value_t &out) const {
  if (!absolute) {
    offset += frame_().sp_;
  }
  if (offset >= 0 && offset < int32_t(s_.size())) {
    out = s_[offset];
    return true;
  }
  return false;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void vm_t::reset() {
  // nothing to do
}
