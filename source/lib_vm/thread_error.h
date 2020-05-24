#pragma once

namespace nano {

// XXX: should add a PC address for a thread error

enum class thread_error_t {
  e_success = 0,
  e_max_cycle_count,
  e_bad_prepare,
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

const char *get_thread_error(const nano::thread_error_t &err);

} // namespace nano
