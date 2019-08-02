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
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v + r->v));
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    stack_.push(gc_->new_float(l->as_float() + r->as_float()));
    return;
  }
  if (l->is_string()) {
    char rbuf[16] = {0};
    to_string(rbuf, sizeof(rbuf), r);
    size_t rsize = strlen(rbuf);
    const int32_t len = l->strlen() + rsize;
    value_t *s = gc_->new_string(len);
    char *dst = s->string();
    memcpy(dst, l->string(), l->strlen());
    memcpy(dst + l->strlen(), rbuf, rsize);
    dst[len] = '\0';
    stack_.push(s);
    return;
  }
  if (r->is_string()) {
    char lbuf[16] = {0};
    to_string(lbuf, sizeof(lbuf), l);
    size_t lsize = strlen(lbuf);
    const int32_t len = lsize + r->strlen();
    value_t *s = gc_->new_string(len);
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
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v - r->v));
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    stack_.push(gc_->new_float(l->as_float() - r->as_float()));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_MUL_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v * r->v));
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    stack_.push(gc_->new_float(l->as_float() * r->as_float()));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_DIV_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    if (r->v == 0) {
      set_error_(thread_error_t::e_bad_divide_by_zero);
    } else {
      const int32_t o = l->v / r->v;
      stack_.push(gc_->new_int(o));
    }
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    stack_.push(gc_->new_float(l->as_float() / r->as_float()));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_MOD_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    if (r->v == 0) {
      set_error_(thread_error_t::e_bad_divide_by_zero);
    } else {
      const int32_t o = l->v % r->v;
      stack_.push(gc_->new_int(o));
    }
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_AND_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v && r->v));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_OR_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v || r->v));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_NOT_() {
  const value_t *l = stack_.pop();

  // convert other types to bool first?

  if (l->is_int()) {
    stack_.push(gc_->new_int(!l->v));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_NEG_() {
  const value_t *o = stack_.pop();
  if (o->is_int()) {
    stack_.push(gc_->new_int(-o->v));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_LT_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v < r->v ? 1 : 0));
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    stack_.push(gc_->new_int(
      l->as_float() < r->as_float() ? 1 : 0));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_GT_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v > r->v ? 1 : 0));
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    stack_.push(gc_->new_int(
      l->as_float() > r->as_float() ? 1 : 0));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_LEQ_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v <= r->v ? 1 : 0));
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    stack_.push(gc_->new_int(
      l->as_float() <= r->as_float() ? 1 : 0));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_GEQ_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v >= r->v ? 1 : 0));
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    stack_.push(gc_->new_int(
      l->as_float() >= r->as_float() ? 1 : 0));
    return;
  }
  raise_error(thread_error_t::e_bad_type_operation);
}

void thread_t::do_INS_EQ_() {
  const value_t *r = stack_.pop();
  const value_t *l = stack_.pop();
  if (l->is_int() && r->is_int()) {
    stack_.push(gc_->new_int(l->v == r->v ? 1 : 0));
    return;
  }
  if (l->is_string() && r->is_string()) {
    int32_t res = 0;
    if (l->strlen() == r->strlen()) {
      if (strcmp(l->string(), r->string()) == 0) {
        res = 1;
      }
    }
    stack_.push(gc_->new_int(res));
    return;
  }
  // only none == none
  if (l->is_none() || r->is_none()) {
    stack_.push(gc_->new_int(l->is_none() && r->is_none() ? 1 : 0));
    return;
  }
  if ((l->is_float() || l->is_int()) &&
      (r->is_float() || r->is_int())) {
    // XXX: use epsilon here?
    stack_.push(gc_->new_int(
      l->as_float() == r->as_float() ? 1 : 0));
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
  switch (o->type()) {
  case value_type_t::val_type_int:
    if (o->v != 0) {
      pc_ = operand;
    }
    break;
  case value_type_t::val_type_string:
    if (o->strlen() != 0) {
      pc_ = operand;
    }
    break;
  case value_type_t::val_type_none:
    break;
  default:
    raise_error(thread_error_t::e_bad_type_operation);
  }
}

void thread_t::do_INS_FJMP_() {
  const int32_t operand = read_operand_();
  const value_t *o = stack_.pop();
  switch (o->type()) {
  case val_type_none:
    pc_ = operand;
    break;
  case val_type_int:
    if (o->v == 0) {
      pc_ = operand;
    }
    break;
  case val_type_string:
    if (o->strlen() == 0) {
      pc_ = operand;
    }
    break;
  default:
    raise_error(thread_error_t::e_bad_type_operation);
  }
}

void thread_t::do_INS_CALL_() {
  const int32_t operand = read_operand_();
  // new frame
  enter_(stack_.head(), pc_);
  pc_ = operand;
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

void thread_t::do_INS_SCALL_() {
  const int32_t operand = read_operand_();

  const auto &funcs = ccml_.functions();
  if (operand < 0 || operand >= int32_t(funcs.size())) {
    set_error_(thread_error_t::e_bad_syscall);
  } else {
    const function_t *func = &ccml_.functions()[operand];
    func->sys_(*this);
  }
}

void thread_t::do_INS_POP_() {
  const int32_t operand = read_operand_();
  for (int32_t i = 0; i < operand; ++i) {
    stack_.pop();
  };
}

void thread_t::do_INS_NEW_INT_() {
  value_t *op = gc_->new_int(read_operand_());
  stack_.push(op);
}

void thread_t::do_INS_NEW_STR_() {
  const int32_t index = read_operand_();
  const auto &str_tab = ccml_.strings();
  assert(index < (int32_t)str_tab.size());
  const std::string &s = ccml_.strings()[index];
  stack_.push(gc_->new_string(s));
}

void thread_t::do_INS_NEW_ARY_() {
  const int32_t index = read_operand_();
  assert(index > 0);
  stack_.push(gc_->new_array(index));
}

void thread_t::do_INS_NEW_NONE_() {
  stack_.push(gc_->new_none());
}

void thread_t::do_INS_NEW_FLT_() {
  uint32_t bits = read_operand_();
  float val = *(const float*)(&bits);
  value_t *op = gc_->new_float(val);
  stack_.push(op);
}

void thread_t::do_INS_GLOBALS_() {
  const int32_t operand = read_operand_();
  if (operand) {
    g_.resize(operand);
    memset(g_.data(), 0, sizeof(value_t*) * operand);
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
  if (operand < 0 || operand >= int32_t(g_.size())) {
    set_error_(thread_error_t::e_bad_get_global);
  } else {
    stack_.push(g_[operand]);
  }
}

void thread_t::do_INS_SETG_() {
  const int32_t operand = read_operand_();
  if (operand < 0 || operand >= int32_t(g_.size())) {
    set_error_(thread_error_t::e_bad_set_global);
  } else {
    g_[operand] = stack_.pop();
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
    stack_.push(out ? out : gc_->new_none());
    return;
  }
  if (a->type() == val_type_string) {
    if (index < 0 || index >= (int32_t)a->strlen()) {
      raise_error(thread_error_t::e_bad_array_bounds);
      return;
    }
    const uint32_t ch = (uint8_t)(a->string()[index]);
    stack_.push(gc_->new_int(ch));
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
  a->array()[index] = gc_->copy(*v);
}

namespace {

} // namespace

bool thread_t::prepare(const function_t &func, int32_t argc,
                       const value_t *argv) {
  error_ = thread_error_t::e_success;
  finished_ = true;
  cycles_ = 0;
  halted_ = false;
  stack_.clear();

  if (func.is_syscall()) {
    return false;
  }

  // save the target pc (entry point)
  pc_ = func.pos_;
  if (func.num_args_ != argc) {
    return_code_ = gc_->new_int(-1);
    error_ = thread_error_t::e_bad_num_args;
    return false;
  }

  // push any arguments
  for (int i = 0; i < argc; ++i) {
    stack_.push(gc_->copy(argv[i]));
  }

  // push the initial frame
  enter_(stack_.head(), pc_);

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
  case INS_ADD:      do_INS_ADD_();      break;
  case INS_SUB:      do_INS_SUB_();      break;
  case INS_MUL:      do_INS_MUL_();      break;
  case INS_DIV:      do_INS_DIV_();      break;
  case INS_MOD:      do_INS_MOD_();      break;
  case INS_AND:      do_INS_AND_();      break;
  case INS_OR:       do_INS_OR_();       break;
  case INS_NOT:      do_INS_NOT_();      break;
  case INS_NEG:      do_INS_NEG_();      break;
  case INS_LT:       do_INS_LT_();       break;
  case INS_GT:       do_INS_GT_();       break;
  case INS_LEQ:      do_INS_LEQ_();      break;
  case INS_GEQ:      do_INS_GEQ_();      break;
  case INS_EQ:       do_INS_EQ_();       break;
  case INS_JMP:      do_INS_JMP_();      break;
  case INS_TJMP:     do_INS_TJMP_();     break;
  case INS_FJMP:     do_INS_FJMP_();     break;
  case INS_CALL:     do_INS_CALL_();     break;
  case INS_RET:      do_INS_RET_();      break;
  case INS_SCALL:    do_INS_SCALL_();    break;
  case INS_POP:      do_INS_POP_();      break;
  case INS_NEW_ARY:  do_INS_NEW_ARY_();  break;
  case INS_NEW_INT:  do_INS_NEW_INT_();  break;
  case INS_NEW_STR:  do_INS_NEW_STR_();  break;
  case INS_NEW_NONE: do_INS_NEW_NONE_(); break;
  case INS_NEW_FLT:  do_INS_NEW_FLT_();  break;
  case INS_LOCALS:   do_INS_LOCALS_();   break;
  case INS_GLOBALS:  do_INS_GLOBALS_();  break;
  case INS_GETV:     do_INS_GETV_();     break;
  case INS_SETV:     do_INS_SETV_();     break;
  case INS_GETG:     do_INS_GETG_();     break;
  case INS_SETG:     do_INS_SETG_();     break;
  case INS_GETA:     do_INS_GETA_();     break;
  case INS_SETA:     do_INS_SETA_();     break;
  default:
    set_error_(thread_error_t::e_bad_opcode);
  }
}

void thread_t::enter_(uint32_t sp, uint32_t pc) {
  if (f_head_ >= f_.size()) {
    set_error_(thread_error_t::e_stack_overflow);
  } else {
    ++f_head_;
    frame_().sp_ = sp;
    frame_().pc_ = pc;
  }
}

// return - old PC as return value
uint32_t thread_t::leave_() {
  if (f_head_ <= 0) {
    set_error_(thread_error_t::e_stack_underflow);
    return 0;
  } else {
    const uint32_t ret_pc = frame_().pc_;
    --f_head_;
    // check if we have finished
    finished_ |= (f_head_ == 0);
    return ret_pc;
  }
}

bool thread_t::step_inst() {
  if (finished_) {
    return false;
  }
  step_imp_();
  // check for program termination
  if (!finished_ && !f_head_) {
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
    assert(stack_.head() > 0);
    return_code_ = stack_.pop();
    finished_ = true;
  }
  return !has_error();
}

int32_t thread_t::source_line() const {
  return ccml_.store().get_line(pc_);
}

void thread_t::tick_gc_(int32_t cycles) {
  // XXX: better - when allocated objects are 2x the stack size?
  if ((cycles % 500) == 1) {
    gc_collect();
  }
}

bool thread_t::resume(int32_t cycles, bool trace) {
  if (finished_) {
    return false;
  }
  const uint32_t start_cycles = cycles;

  if (trace) {
    // while we should keep processing instructions
    for (; cycles; --cycles) {
      tick_gc_(cycles);
      // print an instruction trace
      const uint8_t *c = ccml_.code();
      ccml_.disassembler().disasm(c + pc_);
      // dispatch
      step_imp_();
      if (finished_ || halted_) {
        break;
      }
    }
  } else {
    // while we should keep processing instructions
    for (; cycles; --cycles) {
      tick_gc_(cycles);
      step_imp_();
      if (finished_ || halted_) {
        break;
      }
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

bool thread_t::active_vars(std::vector<const identifier_t *> &out) const {
  return ccml_.store().active_vars(pc_, out);
}

#if 1
// XXX: remove this function
bool vm_t::execute(const function_t &func, int32_t argc, const value_t *argv,
                   value_t *ret, bool trace, thread_error_t *err) {
  if (err) {
    *err = thread_error_t::e_success;
  }
  thread_t t{ccml_};

  if (!t.init()) {
    return false;
  }

  if (!t.prepare(func, argc, argv)) {
    if (err) {
      *err = t.error_;
    }
    return false;
  }
  if (!t.resume(INT_MAX, trace)) {
    if (err) {
      *err = t.error_;
    }
    return false;
  }
  if (!t.finished()) {
    if (err) {
      *err = thread_error_t::e_max_cycle_count;
    }
    return false;
  }
  if (ret) {
    value_t *r = t.return_code();
    if (r->is_none()) {
      ret->type_ = val_type_none;
    } else {
      if (r->is_int()) {
        ret->v = r->v;
      } else {
        r->v = 0;
      }
    }
  }
  return true;
}
#endif

value_t *thread_t::getv_(int32_t offs) {
  const int32_t index = frame_().sp_ + offs;
#if 1
  return stack_.get(index);
#else
  if (index < 0 || index >= int32_t(s_head_)) {
    set_error_(thread_error_t::e_bad_getv);
    return gc_->new_none();
  }
  value_t *v = s_[index];
  return v ? v : gc_->new_none();
#endif
}

void thread_t::setv_(int32_t offs, value_t *val) {
  const int32_t index = frame_().sp_ + offs;
#if 1
  stack_.set(index, val);
#else
  if (index < 0 || index >= int32_t(stack_.head())) {
    set_error_(thread_error_t::e_bad_setv);
  } else {
    s_[index] = val;
  }
#endif
}

void vm_t::reset() {
  // nothing to do
}

void thread_t::gc_collect() {
  gc_->trace(stack_.data(), stack_.head());
  gc_->trace(g_.data(), g_.size());
  gc_->collect();
}

bool thread_t::init() {
  error_ = thread_error_t::e_success;
  finished_ = true;
  cycles_ = 0;
  halted_ = false;

  stack_.clear();

  // search for the init function
  const function_t *init = ccml_.find_function("@init");
  if (!init) {
    // no init to do?
    return true;
  }
  // save the target pc (entry point)
  pc_ = init->pos_;
  // push the initial frame
  enter_(stack_.head(), pc_);
  // catch any misc errors
  if (error_ != thread_error_t::e_success) {
    return false;
  }
  // good to go
  finished_ = false;
  if (!resume(1024 * 8, false)) {
    return false;
  }
  return finished();
}
