#include "value.h"
#include "vm.h"

namespace ccml {

value_stack_t::value_stack_t(thread_t &thread, value_gc_t &gc)
  : head_(0)
  , thread_(thread)
  , gc_(gc)
{
}

void value_stack_t::push_int(const int32_t v) {
  push(gc_.new_int(v));
}

void value_stack_t::push_string(const std::string &v) {
  const size_t len = v.size();
  value_t *s = gc_.new_string(len);
  memcpy(s->string(), v.data(), len);
  s->string()[len] = '\0';
  push(s);
}

void value_stack_t::set_error(thread_error_t error) {
  thread_.raise_error(error);
}

} // namespace ccml
