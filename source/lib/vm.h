#pragma once
#include <vector>
#include <array>
#include <bitset>
#include <string>
#include <set>
#include <string.h>
#include <stdlib.h>

#include "ccml.h"
#include "vm_gc.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
enum class thread_error_t {
  e_success = 0,
  e_max_cycle_count,
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
  e_bad_array_bounds,
  e_bad_array_index,
  e_bad_array_object,
  e_bad_type_operation,
  e_bad_argument,
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct thread_t {

  // XXXX: as it stands globals are not shared among threads

  thread_t(ccml_t &ccml)
    : ccml_(ccml)
    , return_code_(nullptr)
    , finished_(true)
    , cycles_(0)
    , s_head_(0)
    , f_head_(0) {
    gc_.reset(new value_gc_t());
  }

  // peek a stack value
  bool peek(int32_t offset, bool absolute, value_t *&out) const;

  // pop from the value stack
  value_t* pop() {
    return pop_();
  }

  // push integer onto the value stack
  void push_int(const int32_t v) {
    push_(gc_->new_int(v));
  }

  // push string onto the value stack
  void push_string(const std::string &v) {
    const size_t len = v.size();
    value_t *s = gc_->new_string(len);
    memcpy(s->string(), v.data(), len);
    s->string()[len] = '\0';
    push_(s);
  }

  // push onto the value stack
  void push(value_t* v) {
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
  value_t* return_code() const {
    return finished_ ? return_code_ : nullptr;
  }

  // return the total cycle count
  uint32_t cycle_count() const {
    return cycles_;
  }

  bool has_error() const {
    return error_ != thread_error_t::e_success;
  }

  const thread_error_t error() const {
    return error_;
  }

  // return the current source line number
  int32_t source_line() const;

  // collect all currently active variables
  bool active_vars(std::vector<const identifier_t *> &out) const;

  // raise a thread error
  void raise_error(thread_error_t e) {
    error_ = e;
    finished_ = true;
  }

  // return garbage collector
  value_gc_t &gc() {
    return *gc_;
  }

  // execute thread init function
  bool init();

protected:

  void tick_gc_(int32_t cycles);

  friend struct vm_t;

  // step a single instruction (internal)
  void step_imp_();

  // ccml service locator
  ccml_t &ccml_;

  // the return value of the thread function
  value_t* return_code_;

  // set when a thread raises and error
  thread_error_t error_;

  // total cycle count for thread execution
  uint32_t cycles_;

  // true when a thread has finished executing
  bool finished_;

  // syscalls can set to true to halt execution
  bool halted_;

  // program counter
  int32_t pc_;

  // garbage collector
  std::unique_ptr<value_gc_t> gc_;
  void gc_collect();

  // globals
  // XXX: provide a way to share these amongst threads
  std::vector<value_t*> g_;

  // value stack
  uint32_t s_head_;
  std::array<value_t *, 1024 * 8> s_;

  // frame stack
  struct frame_t {
    int32_t sp_;
    int32_t pc_;
  };
  uint32_t f_head_;
  std::array<frame_t, 64> f_;

  // frame control
  void enter_(uint32_t sp, uint32_t pc);
  uint32_t leave_();

  // current stack frame
  const frame_t &frame_() const {
    assert(f_head_);
    return f_[f_head_-1];
  }

  // current stack frame
  frame_t &frame_() {
    assert(f_head_);
    return f_[f_head_-1];
  }

  // push value onto stack
  void push_(value_t *v) {
    if (s_head_ >= s_.size()) {
      set_error_(thread_error_t::e_stack_overflow);
    } else {
      s_[s_head_++] = v;
    }
  }

  // peek a value on the stack
  value_t* peek_() {
    if (s_head_ <= 0) {
      set_error_(thread_error_t::e_stack_underflow);
      return gc_->new_int(0);
    } else {
      return s_[s_head_ - 1];
    }
  }

  // pop value from stack
  value_t* pop_() {
    if (s_head_ <= 0) {
      set_error_(thread_error_t::e_stack_underflow);
      return gc_->new_int(0);
    } else {
      return s_[--s_head_];
    }
  }

  // raise an error
  void set_error_(thread_error_t error) {
    finished_ = true;
    error_ = error;
    return_code_ = gc_->new_int(-1);
  }

  value_t* getv_(int32_t offs);
  void setv_(int32_t offs, value_t* val);

  int32_t read_operand_();
  uint8_t read_opcode_();
  uint8_t peek_opcode_();

  void do_INS_ADD_();
  void do_INS_SUB_();
  void do_INS_MUL_();
  void do_INS_DIV_();
  void do_INS_MOD_();
  void do_INS_AND_();
  void do_INS_OR_();
  void do_INS_NOT_();
  void do_INS_NEG_();
  void do_INS_LT_();
  void do_INS_GT_();
  void do_INS_LEQ_();
  void do_INS_GEQ_();
  void do_INS_EQ_();
  void do_INS_JMP_();
  void do_INS_TJMP_();
  void do_INS_FJMP_();
  void do_INS_CALL_();
  void do_INS_RET_();
  void do_INS_SCALL_();
  void do_INS_POP_();
  void do_INS_NEW_INT_();
  void do_INS_NEW_STR_();
  void do_INS_NEW_ARY_();
  void do_INS_NEW_NONE_();
  void do_INS_LOCALS_();
  void do_INS_GLOBALS_();
  void do_INS_GETV_();
  void do_INS_SETV_();
  void do_INS_GETG_();
  void do_INS_SETG_();
  void do_INS_GETA_();
  void do_INS_SETA_();
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct vm_t {

  vm_t(ccml_t &c)
    : ccml_(c) {}

#if 1
  // XXX: remove this function
  bool execute(const function_t &func, int32_t argc, const value_t *argv,
               value_t *ret = nullptr, bool trace = false, thread_error_t *err = nullptr);
#endif

  // reset any stored state
  void reset();

protected:
  ccml_t &ccml_;
};

} // namespace ccml
