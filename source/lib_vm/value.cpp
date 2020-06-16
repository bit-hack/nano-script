#include "value.h"
#include "vm.h"
#include "thread.h"

namespace nano {

value_stack_t::value_stack_t(thread_t &thread, value_gc_t &gc)
  : thread_(thread)
  , gc_(gc)
{
  stack_.reserve(128);
}

void value_stack_t::push_none() {
  push(gc_.new_none());
}

void value_stack_t::push_func(const int32_t address) {
  push(gc_.new_func(address));
}

void value_stack_t::push_syscall(const int32_t index) {
  push(gc_.new_syscall(index));
}

void value_stack_t::push_int(const int32_t v) {
  push(gc_.new_int(v));
}

void value_stack_t::push_float(const float v) {
  push(gc_.new_float(v));
}

void value_stack_t::push_string(const std::string &v) {
  const int32_t len = int32_t(v.size());
  value_t *s = gc_.new_string(len);
  memcpy(s->string(), v.data(), len);
  s->string()[len] = '\0';
  push(s);
}

void value_stack_t::set_error(thread_error_t error) {
  thread_.raise_error(error);
}

std::string value_t::to_string() const {
  switch (this ? type_ : val_type_none) {
  case val_type_int:     return std::to_string(v);
  case val_type_float:   return std::to_string(f);
  case val_type_string:  return "\"" + std::string(string()) + "\"";
  case val_type_array:   return "array@" + std::to_string((uint64_t)(void*)this);
  case val_type_none:    return "none";
  case val_type_func:    return "function@" + std::to_string(v);
  case val_type_syscall: return "syscall@" + std::to_string(v);
  default:
    assert(!"unknown");
    return "";
  }
}

} // namespace nano
