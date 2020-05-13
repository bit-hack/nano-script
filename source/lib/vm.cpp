#include <climits>
#include <cstring>

#include "codegen.h"
#include "disassembler.h"
#include "parser.h"
#include "vm.h"

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

namespace {

bool to_string(char *buf, size_t size, const value_t *val) {
  switch (val->type()) {
  case val_type_float:
    snprintf(buf, size, "%f", val->f);
    return true;
  case val_type_int:
    snprintf(buf, size, "%i", val->integer());
    return true;
  case val_type_string:
    snprintf(buf, size, "%s", val->string());
    return true;
  case val_type_none:
    snprintf(buf, size, "none");
    return true;
  case val_type_func:
    snprintf(buf, size, "function@%d", val->v);
    return true;
  default:
    assert(false);
    return false;
  }
}

} // namespace {}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
namespace {

int32_t history[__INS_COUNT__];

} // namespace

void print_history() {
  printf("histogram:\n");
  for (int32_t i = 0; i < __INS_COUNT__; ++i) {
    const char *name = disassembler_t::get_mnemonic((instruction_e)i);
    printf("  %12s %d\n", name, history[i]);
  }
}

thread_t::thread_t(vm_t &vm)
  : return_code_(nullptr)
  , cycles_(0)
  , finished_(true)
  , halted_(false)
  , pc_(0)
  , vm_(vm)
  , gc_(*(vm.gc_))
  , stack_(*this, *(vm.gc_))
{
  f_.reserve(16);
  vm_.threads_.insert(this);
  reset();
}

thread_t::~thread_t() {
  auto itt = vm_.threads_.find(this);
  assert(itt != vm_.threads_.end());
  vm_.threads_.erase(itt);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
int32_t thread_t::read_operand_() {
  const uint8_t *c = vm_.program_.data();
  assert(sizeof(pc_) < vm_.program_.size());
  const int32_t val = *(int32_t *)(c + pc_);
  pc_ += sizeof(int32_t);
  return val;
}

uint8_t thread_t::peek_opcode_() {
  const uint8_t *c = vm_.program_.data();
  assert(sizeof(pc_) < vm_.program_.size());
  const int8_t val = *(uint8_t *)(c + pc_);
  return val;
}

uint8_t thread_t::read_opcode_() {
  const uint8_t *c = vm_.program_.data();
  assert(sizeof(pc_) < vm_.program_.size());
  const int8_t val = *(uint8_t *)(c + pc_);
  pc_ += sizeof(uint8_t);
  return val;
}

void thread_t::do_INS_ADD_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    stack_.push_int(l->v + r->v);
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    stack_.push_float(l->as_float() + r->as_float());
    return;
  }
  if (l->is_a<val_type_string>()) {
    char rbuf[16] = {0};
    to_string(rbuf, sizeof(rbuf), r);
    size_t rsize = strlen(rbuf);
    const int32_t len = l->strlen() + rsize;
    value_t *s = gc_.new_string(len);
    char *dst = s->string();
    memcpy(dst, l->string(), l->strlen());
    memcpy(dst + l->strlen(), rbuf, rsize);
    dst[len] = '\0';
    stack_.push(s);
    return;
  }
  if (r->is_a<val_type_string>()) {
    char lbuf[16] = {0};
    to_string(lbuf, sizeof(lbuf), l);
    size_t lsize = strlen(lbuf);
    const int32_t len = lsize + r->strlen();
    value_t *s = gc_.new_string(len);
    char *dst = s->string();
    memcpy(dst, lbuf, lsize);
    memcpy(dst + lsize, r->string(), r->strlen());
    dst[len] = '\0';
    stack_.push(s);
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_SUB_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    stack_.push_int(l->v - r->v);
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    stack_.push_float(l->as_float() - r->as_float());
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_MUL_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() && r->is_a<val_type_int>()) {
    stack_.push_int(l->v * r->v);
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    stack_.push_float(l->as_float() * r->as_float());
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_DIV_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    if (r->v == 0) {
      set_error_(thread_error_t::e_bad_divide_by_zero);
    } else {
      const int32_t o = l->v / r->v;
      stack_.push_int(o);
    }
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    stack_.push_float(l->as_float() / r->as_float());
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_MOD_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    if (r->v == 0) {
      set_error_(thread_error_t::e_bad_divide_by_zero);
    } else {
      const int32_t o = l->v % r->v;
      stack_.push_int(o);
    }
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_AND_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  stack_.push_int(l->as_bool() && r->as_bool());
}

void thread_t::do_INS_OR_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  stack_.push_int(l->as_bool() || r->as_bool());
}

void thread_t::do_INS_NOT_() {
  const value_t *l = stack_.pop();
  stack_.push_int(!l->as_bool());
}

void thread_t::do_INS_NEG_() {
  const value_t *o = stack_.pop();
  if (o->is_a<val_type_int>()) {
    stack_.push_int(-o->v);
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_LT_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    stack_.push_int(l->v < r->v ? 1 : 0);
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    stack_.push_int(
      l->as_float() < r->as_float() ? 1 : 0);
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_GT_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    stack_.push_int(l->v > r->v ? 1 : 0);
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    stack_.push_int(
      l->as_float() > r->as_float() ? 1 : 0);
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_LEQ_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    stack_.push_int(l->v <= r->v ? 1 : 0);
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    stack_.push_int(
      l->as_float() <= r->as_float() ? 1 : 0);
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_GEQ_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    stack_.push_int(l->v >= r->v ? 1 : 0);
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    stack_.push_int(
      l->as_float() >= r->as_float() ? 1 : 0);
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_EQ_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_a<val_type_int>() &&
      r->is_a<val_type_int>()) {
    stack_.push_int(l->v == r->v ? 1 : 0);
    return;
  }
  if (l->is_a<val_type_string>() &&
      r->is_a<val_type_string>()) {
    int32_t res = 0;
    if (l->strlen() == r->strlen()) {
      if (strcmp(l->string(), r->string()) == 0) {
        res = 1;
      }
    }
    stack_.push_int(res);
    return;
  }
  if (l->is_a<val_type_func>() &&
      r->is_a<val_type_func>()) {
    stack_.push_int((l->v == r->v) ? 1 : 0);
    return;
  }
  // only none == none
  if (l->is_a<val_type_none>() ||
      r->is_a<val_type_none>()) {
    stack_.push_int(l->is_a<val_type_none>() &&
                    r->is_a<val_type_none>() ? 1 : 0);
    return;
  }
  if ((l->is_a<val_type_float>() || l->is_a<val_type_int>()) &&
      (r->is_a<val_type_float>() || r->is_a<val_type_int>())) {
    // XXX: use epsilon here?
    stack_.push_int(
      l->as_float() == r->as_float() ? 1 : 0);
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_JMP_() {
  pc_ = read_operand_();
}

void thread_t::do_INS_TJMP_() {
  const int32_t operand = read_operand_();
  const value_t *o = stack_.pop();
  if (o->as_bool()) {
    pc_ = operand;
  }
}

void thread_t::do_INS_FJMP_() {
  const int32_t operand = read_operand_();
  const value_t *o = stack_.pop();
  if (!o->as_bool()) {
    pc_ = operand;
  }
}

void thread_t::do_INS_CALL_() {
  const int32_t num_args = read_operand_();
  (void)num_args;
  const int32_t callee = read_operand_();
  // new frame
  enter_(stack_.head(), pc_, callee);
}

void thread_t::do_INS_RET_() {
  const int32_t operand = read_operand_();
  // pop return value
  value_t *sval = stack_.pop();
  // remove arguments and local vars
  if (int32_t(stack_.head()) < operand) {
    set_error_(thread_error_t::e_stack_underflow);
  } else {
    stack_.discard(operand);
    // push return value
    stack_.push(sval);
    // return to previous frame
    pc_ = leave_();
  }
}

void thread_t::do_syscall_(int32_t operand, int32_t num_args) {
  const auto &calls = vm_.program_.syscall();
  assert(operand >= 0 && operand < int32_t(calls.size()));
  ccml_syscall_t sys = calls[operand];
  assert(sys);
  sys(*this, num_args);
}

void thread_t::do_INS_SCALL_() {
  const int32_t num_args = read_operand_();
  const int32_t operand = read_operand_();
  do_syscall_(operand, num_args);
}

void thread_t::do_INS_ICALL_() {
  const int32_t num_args = read_operand_();
  value_t *callee = stack_.pop();
  assert(callee);
  if (callee->is_a<val_type_syscall>()) {
    const int32_t operand = callee->v;
    do_syscall_(operand, num_args);
    return;
  }
  if (callee->is_a<val_type_func>()) {
    const int32_t addr = callee->v;
    // check number of arguments given to a function
    const function_t *func = vm_.program_.function_find(addr);
    assert(func);
    if (int32_t(func->num_args()) != num_args) {
      set_error_(thread_error_t::e_bad_num_args);
    }
    // new frame
    enter_(stack_.head(), pc_, addr);
    return;
  }
  set_error_(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_POP_() {
  const int32_t operand = read_operand_();
  for (int32_t i = 0; i < operand; ++i) {
    stack_.pop();
  };
}

void thread_t::do_INS_NEW_INT_() {
  value_t *op = gc_.new_int(read_operand_());
  stack_.push(op);
}

void thread_t::do_INS_NEW_STR_() {
  const int32_t index = read_operand_();
  const auto &str_tab = vm_.program_.strings();
  (void)str_tab;
  assert(index < (int32_t)str_tab.size());
  const std::string &s = str_tab[index];
  stack_.push_string(s);
}

void thread_t::do_INS_NEW_ARY_() {
  const int32_t index = read_operand_();
  assert(index > 0);
  stack_.push(gc_.new_array(index));
}

void thread_t::do_INS_NEW_NONE_() {
  stack_.push_none();
}

void thread_t::do_INS_NEW_FLT_() {
  uint32_t bits = read_operand_();
  float val = *(const float*)(&bits);
  value_t *op = gc_.new_float(val);
  stack_.push(op);
}

void thread_t::do_INS_NEW_FUNC_() {
  const int32_t index = read_operand_();
  assert(index >= 0);
  stack_.push_func(index);
}

void thread_t::do_INS_NEW_SCALL_() {
  const int32_t index = read_operand_();
  assert(index >= 0);
  stack_.push_syscall(index);
}

void thread_t::do_INS_GLOBALS_() {
  const int32_t operand = read_operand_();
  if (operand) {
    vm_.g_.resize(operand);
    memset(vm_.g_.data(), 0, sizeof(value_t*) * operand);
  }
}

void thread_t::do_INS_LOCALS_() {
  const int32_t operand = read_operand_();
  if (operand) {
    stack_.reserve(operand);
  }
}

void thread_t::do_INS_GETV_() {
  const int32_t operand = read_operand_();
  stack_.push(getv_(operand));
}

void thread_t::do_INS_SETV_() {
  const int32_t operand = read_operand_();
  setv_(operand, stack_.pop());
}

void thread_t::do_INS_GETG_() {
  const int32_t operand = read_operand_();
  if (operand < 0 || operand >= int32_t(vm_.g_.size())) {
    set_error_(thread_error_t::e_bad_get_global);
  } else {
    stack_.push(vm_.g_[operand]);
  }
}

void thread_t::do_INS_SETG_() {
  const int32_t operand = read_operand_();
  if (operand < 0 || operand >= int32_t(vm_.g_.size())) {
    set_error_(thread_error_t::e_bad_set_global);
  } else {
    vm_.g_[operand] = stack_.pop();
  }
}

void thread_t::do_INS_GETA_() {
  value_t *a = stack_.pop();
  value_t *i = stack_.pop();
  if (!a) {
    raise_error(thread_error_t::e_bad_array_object);
    return;
  }
  if (i->type() != val_type_int) {
    raise_error(thread_error_t::e_bad_array_index);
    return;
  }
  const int32_t index = i->v;
  if (a->type() == val_type_array) {
    if (index < 0 || index >= a->array_size()) {
      raise_error(thread_error_t::e_bad_array_bounds);
      return;
    }
    value_t *out = a->array()[index];
    stack_.push(out ? out : gc_.new_none());
    return;
  }
  if (a->type() == val_type_string) {
    if (index < 0 || index >= (int32_t)a->strlen()) {
      raise_error(thread_error_t::e_bad_array_bounds);
      return;
    }
    const uint32_t ch = (uint8_t)(a->string()[index]);
    stack_.push_int(ch);
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_SETA_() {
  value_t *a = stack_.pop();
  value_t *i = stack_.pop();
  value_t *v = stack_.pop();
  if (a->type() != val_type_array) {
    raise_error(thread_error_t::e_bad_array_object);
    return;
  }
  if (i->type() != val_type_int) {
    raise_error(thread_error_t::e_bad_array_index);
    return;
  }
  const int32_t index = i->v;
  if (index < 0 || index >= a->array_size()) {
    raise_error(thread_error_t::e_bad_array_bounds);
    return;
  }
  a->array()[index] = gc_.copy(*v);
}

void thread_t::reset() {
  error_ = thread_error_t::e_success;
  stack_.clear();
  f_.clear();
  halted_ = false;
  finished_ = false;
  return_code_ = nullptr;
}

bool thread_t::prepare(const function_t &func, int32_t argc,
                       const value_t *argv) {

  error_ = thread_error_t::e_success;
  finished_ = true;
  cycles_ = 0;
  halted_ = false;
  return_code_ = nullptr;

  if (func.is_syscall()) {
    return false;
  }

  stack_.clear();

  // load the target pc (entry point)
  pc_ = func.code_start_;

  // verify num arguments
  if (int32_t(func.num_args()) != argc) {
    return_code_ = gc_.new_int(-1);
    error_ = thread_error_t::e_bad_num_args;
    return false;
  }

  // push any arguments
  for (int i = 0; i < argc; ++i) {
    stack_.push(gc_.copy(argv[i]));
  }

  // push the initial frame
  enter_(stack_.head(), pc_, func.code_start_);
  frame_().terminal_ = true;

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
  case INS_ADD:      do_INS_ADD_();        break;
  case INS_SUB:      do_INS_SUB_();        break;
  case INS_MUL:      do_INS_MUL_();        break;
  case INS_DIV:      do_INS_DIV_();        break;
  case INS_MOD:      do_INS_MOD_();        break;
  case INS_AND:      do_INS_AND_();        break;
  case INS_OR:       do_INS_OR_();         break;
  case INS_NOT:      do_INS_NOT_();        break;
  case INS_NEG:      do_INS_NEG_();        break;
  case INS_LT:       do_INS_LT_();         break;
  case INS_GT:       do_INS_GT_();         break;
  case INS_LEQ:      do_INS_LEQ_();        break;
  case INS_GEQ:      do_INS_GEQ_();        break;
  case INS_EQ:       do_INS_EQ_();         break;
  case INS_JMP:      do_INS_JMP_();        break;
  case INS_TJMP:     do_INS_TJMP_();       break;
  case INS_FJMP:     do_INS_FJMP_();       break;
  case INS_CALL:     do_INS_CALL_();       break;
  case INS_RET:      do_INS_RET_();        break;
  case INS_SCALL:    do_INS_SCALL_();      break;
  case INS_ICALL:    do_INS_ICALL_();      break;
  case INS_POP:      do_INS_POP_();        break;
  case INS_NEW_ARY:  do_INS_NEW_ARY_();    break;
  case INS_NEW_INT:  do_INS_NEW_INT_();    break;
  case INS_NEW_STR:  do_INS_NEW_STR_();    break;
  case INS_NEW_NONE: do_INS_NEW_NONE_();   break;
  case INS_NEW_FLT:  do_INS_NEW_FLT_();    break;
  case INS_NEW_FUNC: do_INS_NEW_FUNC_();   break;
  case INS_NEW_SCALL: do_INS_NEW_SCALL_(); break;
  case INS_LOCALS:   do_INS_LOCALS_();     break;
  case INS_GLOBALS:  do_INS_GLOBALS_();    break;
  case INS_GETV:     do_INS_GETV_();       break;
  case INS_SETV:     do_INS_SETV_();       break;
  case INS_GETG:     do_INS_GETG_();       break;
  case INS_SETG:     do_INS_SETG_();       break;
  case INS_GETA:     do_INS_GETA_();       break;
  case INS_SETA:     do_INS_SETA_();       break;
  default:
    set_error_(thread_error_t::e_bad_opcode);
  }
}

void thread_t::enter_(uint32_t sp, uint32_t pc, uint32_t callee) {
  f_.emplace_back();
  frame_().sp_ = sp;
  frame_().return_ = pc;
  frame_().terminal_ = false;
  frame_().callee_ = callee;
  pc_ = callee;
}

// return - old PC as return value
uint32_t thread_t::leave_() {
  if (f_.empty()) {
    set_error_(thread_error_t::e_stack_underflow);
    return 0;
  } else {
    const uint32_t ret_pc = frame_().return_;
    if (frame_().terminal_) {
      // XXX: should set a status so caller know we finished
      finished_ = true;
    }
    f_.pop_back();
    // check if we have more frames
    finished_ |= f_.empty();
    return ret_pc;
  }
}

bool thread_t::step_inst() {
  if (finished_) {
    return false;
  }
  step_imp_();
  // check for program termination
  if (!finished_ && !f_.empty()) {
    assert(stack_.head() > 0);
    return_code_ = stack_.pop();
    finished_ = true;
  }
  return !has_error();
}

bool thread_t::step_line() {
  if (finished_) {
    return false;
  }
  // get the current source line
  const line_t line = source_line();
  // step until the source line changes
  do {
    step_imp_();
    if (finished_ || !f_.empty() || has_error()) {
      break;
    }
  } while (line == source_line());
  // check for program termination
  if (!finished_ && !f_.empty()) {
    assert(stack_.head() > 0);
    return_code_ = stack_.pop();
    finished_ = true;
  }
  return !has_error();
}

line_t thread_t::source_line() const {
  return vm_.program_.get_line(pc_);
}

void thread_t::tick_gc_(int32_t cycles) {
  (void)cycles;
  if (gc().should_collect()) {
    vm_.gc_collect();
  }
}

bool thread_t::resume(int32_t cycles) {
  if (finished_) {
    return false;
  }
  const uint32_t start_cycles = cycles;
  halted_ = false;

  // while we should keep processing instructions
  for (; cycles; --cycles) {
    tick_gc_(cycles);
    step_imp_();
    if (finished_ || halted_) {
      break;
    }
  }

  // check for program termination
  if (finished_) {
    return_code_ = stack_.peek();
  }

  // increment the cycle count
  cycles_ += (start_cycles - cycles);
  return !has_error();
}

value_t *thread_t::getv_(int32_t offs) {
  const int32_t index = frame_().sp_ + offs;
  return stack_.get(index);
}

void thread_t::setv_(int32_t offs, value_t *val) {
  const int32_t index = frame_().sp_ + offs;
  stack_.set(index, val);
}

bool thread_t::init() {
  error_ = thread_error_t::e_success;
  finished_ = true;
  cycles_ = 0;
  halted_ = false;

  stack_.clear();

  // search for the init function
  const function_t *init = vm_.program_.function_find("@init");
  if (!init) {
    // no init to do?
    return true;
  }
  // save the target pc (entry point)
  pc_ = init->code_start_;
  // push the initial frame
  enter_(stack_.head(), pc_, init->code_start_);
  frame_().terminal_ = true;
  // catch any misc errors
  if (error_ != thread_error_t::e_success) {
    return false;
  }
  // good to go
  finished_ = false;
  if (!resume(1024 * 8)) {
    return false;
  }
  return finished();
}

void thread_t::unwind() {
  int32_t i = 0;
  for (auto itt = f_.rbegin(); itt != f_.rend(); ++itt) {
    const auto &frame = *itt;
    const function_t *func = vm_.program_.function_find(frame.callee_);
    assert(func);
    fprintf(stderr, "%2d> function %s\n", i, func->name_.c_str());
    // print function arguments
    for (const auto &a : func->args_) {
      fprintf(stderr, "  - %4s: ", a.name_.c_str());
      const int32_t index = frame.sp_ + a.offset_;
      const value_t *v = stack().get(index);
      fprintf(stderr, "%s\n", v->to_string().c_str());
    }
    // print local variables
    for (const auto &l : func->locals_) {
      fprintf(stderr, "  - %4s: ", l.name_.c_str());
      const int32_t index = frame.sp_ + l.offset_;
      const value_t *v = stack().get(index);
      fprintf(stderr, "%s\n", v->to_string().c_str());
    }
    // exit on terminal frame
    if (frame.terminal_) {
      break;
    }
    ++i;
  }
}

vm_t::vm_t(program_t &program)
  : program_(program)
  , gc_(new value_gc_t) {
}

void vm_t::gc_collect() {
  // traverse globals
  gc_->trace(g_.data(), g_.size());
  // traverse thread stack
  for (thread_t *t : threads_) {
    gc_->trace(t->stack_.data(), t->stack_.head());
  }
  // collect
  gc_->collect();
}

void vm_t::reset() {

  // reset the garbage collector
  gc_->reset();

  // delete all threads
  for (thread_t *t : threads_) {
    delete t;
  }
  threads_.clear();
}
