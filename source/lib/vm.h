#pragma once
#include <vector>
#include <array>

#include "ccml.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
enum class thread_error_t {
  e_success = 0,
  e_bad_getv,
  e_bad_setv,
  e_bad_num_args,
  e_bad_syscall,
  e_bad_opcode,
  e_bad_set_global,
  e_bad_get_global,
  e_bad_pop,
  e_bad_divide_by_zero,
  e_stack_overflow,
  e_stack_underflow,
  e_bad_globals_size,
};

using value_t = int32_t;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct thread_t {

  // XXXX: as it stands globals are not shared among threads

  thread_t(ccml_t &ccml)
      : ccml_(ccml), return_code_(0), finished_(true), cycles_(0), s_head_(0),
        f_head_(0) {}

  // peek a stack value
  bool peek(int32_t offset, bool absolute, value_t &out) const;

  // pop from the value stack
  value_t pop() {
    return pop_();
  }

  // push onto the value stack
  void push(value_t v) {
    push_(v);
  }

  // prepare to execute a function
  bool prepare(const function_t &func, int32_t argc, const value_t *argv);

  // run for a number of clock cycles
  bool resume(int32_t cycles, bool trace);

  // step a single instruction
  bool step_inst();

  // step a source line
  bool step_line();

  bool finished() const {
    return finished_;
  }

  // return the current error code
  int32_t return_code() const {
    return finished_ ? return_code_ : 0;
  }

  // return the total cycle count
  uint32_t cycle_count() const {
    return cycles_;
  }

  bool has_error() const {
    return error_ != thread_error_t::e_success;
  }

  // return the current source line number
  int32_t source_line() const;

  // collect all currently active variables
  bool active_vars(std::vector<const identifier_t *> &out) const;

protected:
  friend struct vm_t;

  // step a single instruction (internal)
  void step_imp_();

  // XXX: add a halted flag?
  ccml_t &ccml_;
  int32_t return_code_;
  bool finished_;
  thread_error_t error_;
  uint32_t cycles_;

  struct frame_t {
    int32_t sp_;
    int32_t pc_;
  };

  int32_t pc_;                    // program counter
  std::array<value_t, 1024> s_;   // value stack
  std::array<frame_t, 64> f_;     // frame stack
  uint32_t s_head_;
  uint32_t f_head_;

  void enter_(uint32_t sp, uint32_t pc) {
    if (f_head_ >= f_.size()) {
      set_error(thread_error_t::e_stack_overflow);
    }
    else {
      ++f_head_;
      frame_().sp_ = sp;
      frame_().pc_ = pc;
    }
  }

  // return - old PC as return value
  uint32_t leave_() {
    if (f_head_ <= 0) {
      set_error(thread_error_t::e_stack_underflow);
      return 0;
    }
    else {
      const uint32_t ret_pc = frame_().pc_;
      --f_head_;
      return ret_pc;
    }
  }

  const frame_t &frame_() const {
    assert(f_head_);
    return f_[f_head_-1];
  }

  frame_t &frame_() {
    assert(f_head_);
    return f_[f_head_-1];
  }

  void push_(value_t v) {
    if (s_head_ >= s_.size()) {
      set_error(thread_error_t::e_stack_overflow);
    } else {
      s_[s_head_++] = v;
    }
  }

  value_t pop_() {
    if (s_head_ <= 0) {
      set_error(thread_error_t::e_stack_underflow);
      return 0;
    } else {
      return s_[--s_head_];
    }
  }

  void set_error(thread_error_t error) {
    finished_ = true;
    error_ = error;
    return_code_ = -1;
  }

  value_t getv(int32_t offs);
  void setv(int32_t offs, value_t val);

  int32_t _read_operand();
  uint8_t _read_opcode();
  uint8_t _peek_opcode();

  void _do_INS_ADD();
  void _do_INS_SUB();
  void _do_INS_MUL();
  void _do_INS_DIV();
  void _do_INS_MOD();
  void _do_INS_AND();
  void _do_INS_OR();
  void _do_INS_NOT();
  void _do_INS_NEG();
  void _do_INS_LT();
  void _do_INS_GT();
  void _do_INS_LEQ();
  void _do_INS_GEQ();
  void _do_INS_EQ();
  void _do_INS_JMP();
  void _do_INS_CJMP();
  void _do_INS_CALL();
  void _do_INS_RET();
  void _do_INS_SCALL();
  void _do_INS_POP();
  void _do_INS_CONST();
  void _do_INS_LOCALS();
  void _do_INS_ACCV();
  void _do_INS_GETV();
  void _do_INS_SETV();
  void _do_INS_GETVI();
  void _do_INS_SETVI();
  void _do_INS_GETG();
  void _do_INS_SETG();
  void _do_INS_GETGI();
  void _do_INS_SETGI();
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct vm_t {

  vm_t(ccml_t &c)
    : ccml_(c) {}

  bool execute(const function_t &func, int32_t argc, const value_t *argv,
               int32_t *ret = nullptr, bool trace = false);

  // reset any stored state
  void reset();

protected:
  ccml_t &ccml_;
};

} // namespace ccml
