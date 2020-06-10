#pragma once
#include <array>
#include <bitset>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../lib_common/common.h"
#include "../lib_common/types.h"

#include "vm_gc.h"

namespace nano {

struct vm_t;

struct frame_t {
  // stack pointer
  int32_t sp_;

  // return address
  int32_t return_;

  // the function for this frame
  int32_t callee_;

  // we should terminate after this frame
  bool terminal_;
};

struct thread_t {

  thread_t(vm_t &vm);
  ~thread_t();

  // prepare to execute a function
  bool prepare(const function_t &func, int32_t argc, const value_t *argv);

  // run for a number of clock cycles
  bool resume(int32_t cycles);

  // step a single instruction
  bool step_inst();

  // step a source line
  bool step_line();

  bool finished() const { return finished_; }

  // return the current error code
  value_t *get_return_value() const {
    return finished_ ? stack_.peek() : nullptr;
  }

  // return the total cycle count
  uint32_t get_cycle_count() const { return cycles_; }

  bool has_error() const { return error_ != thread_error_t::e_success; }

  thread_error_t get_error() const { return error_; }

  // return the current source line number
  line_t get_source_line() const;

  // raise a thread error
  void raise_error(thread_error_t e) {
    error_ = e;
    finished_ = true;
  }

  // return garbage collector
  value_gc_t &gc() { return gc_; }

  // return the value stack
  value_stack_t &get_stack() { return stack_; }

  // halt this thread
  void halt() { halted_ = true; }

  void reset();

  // return the program counter
  int32_t get_pc() const { return pc_; }

  const std::vector<frame_t> &frames() const { return f_; }

  void breakpoint_add(line_t line);
  void breakpoint_remove(line_t line);
  void breakpoint_clear();

  // space for user data
  void *user_data;

protected:
  friend struct vm_t;

  // when we resume a thread we must keep track of the previous source line.
  // we need to do this so we can resume from a breakpoint without hitting it
  // again.
  line_t last_line_;

  // set when a thread raises and error
  thread_error_t error_;

  // total elapsed cycle count for thread execution
  uint32_t cycles_;

  // true when a thread has finished executing
  bool finished_;

  // syscalls can set to true to halt execution
  bool halted_;

  // program counter
  int32_t pc_;

  // parent virtual machine
  vm_t &vm_;

  // garbage collector from parent vm
  value_gc_t &gc_;

  // frame stack
  std::vector<frame_t> f_;

  friend struct value_stack_t;
  value_stack_t stack_;

  std::set<line_t> breakpoints_;

  // execute thread init function which will initalize any globals
  bool call_init_();

  // tick the garbage collector
  void tick_gc_(int32_t cycles);

  // step a single instruction (internal)
  void step_imp_();

  // frame control
  void enter_(uint32_t sp, uint32_t ret, uint32_t callee);
  uint32_t leave_();

  // syscall helper
  void do_syscall_(int32_t index, int32_t num_args);

  // current stack frame
  const frame_t &frame_() const {
    assert(!f_.empty());
    return f_.back();
  }

  // current stack frame
  frame_t &frame_() {
    assert(!f_.empty());
    return f_.back();
  }

  // raise an error
  void set_error_(thread_error_t error) {
    finished_ = true;
    error_ = error;
  }

  value_t *getv_(int32_t offs);
  void setv_(int32_t offs, value_t *val);

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
  void do_INS_ICALL_();
  void do_INS_POP_();
  void do_INS_NEW_STR_();
  void do_INS_NEW_ARY_();
  void do_INS_NEW_NONE_();
  void do_INS_NEW_INT_();
  void do_INS_NEW_FLT_();
  void do_INS_NEW_FUNC_();
  void do_INS_NEW_SCALL_();
  void do_INS_LOCALS_();
  void do_INS_GLOBALS_();
  void do_INS_GETV_();
  void do_INS_SETV_();
  void do_INS_GETG_();
  void do_INS_SETG_();
  void do_INS_GETA_();
  void do_INS_SETA_();
  void do_INS_GETM_();
  void do_INS_SETM_();
};

} // namespace nano
