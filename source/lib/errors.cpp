#include <cstdarg>
#include <cstdio>

#include "errors.h"

using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void error_manager_t::on_error_(uint32_t line, const char *fmt, ...) {
  // generate the error message
  char buffer[1024] = {'\0'};
  va_list va;
  va_start(va, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, va);
  buffer[sizeof(buffer) - 1] = '\0';
  va_end(va);
  // throw an error
  error_t error{buffer, line};
  throw error;
}

namespace ccml {
const char *get_thread_error(const ccml::thread_error_t &err) {
  using namespace ccml;
  switch (err) {
  case thread_error_t::e_success:
    return "e_success";
  case thread_error_t::e_max_cycle_count:
    return "e_max_cycle_count";
  case thread_error_t::e_bad_getv:
    return "e_bad_getv";
  case thread_error_t::e_bad_setv:
    return "e_bad_setv";
  case thread_error_t::e_bad_num_args:
    return "e_bad_num_args";
  case thread_error_t::e_bad_syscall:
    return "e_bad_syscall";
  case thread_error_t::e_bad_opcode:
    return "e_bad_opcode";
  case thread_error_t::e_bad_set_global:
    return "e_bad_set_global";
  case thread_error_t::e_bad_get_global:
    return "e_bad_get_global";
  case thread_error_t::e_bad_pop:
    return "e_bad_pop";
  case thread_error_t::e_bad_divide_by_zero:
    return "e_bad_divide_by_zero";
  case thread_error_t::e_stack_overflow:
    return "e_stack_overflow";
  case thread_error_t::e_stack_underflow:
    return "e_stack_underflow";
  case thread_error_t::e_bad_globals_size:
    return "e_bad_globals_size";
  case thread_error_t::e_bad_array_bounds:
    return "e_bad_array_bounds";
  case thread_error_t::e_bad_type_operation:
    return "e_bad_type_operation";
  case thread_error_t::e_bad_argument:
    return "e_bad_argument";
  default:
    assert(false);
    return "";
  }
}
}  // namespace ccml
