#include "ccml.h"
#include "vm.h"

// TODO: shl, shr

namespace ccml {

void builtin_abs(struct ccml::thread_t &thread) {
  const ccml::value_t *v = thread.pop();
  assert(v);
  if (v->is_int()) {
    const int64_t res = (v->v < 0) ? (-v->v) : v->v;
    thread.push_int(res);
  } else {
    thread.raise_error(thread_error_t::e_bad_argument);
  }
}

void builtin_max(struct ccml::thread_t &thread) {
  const ccml::value_t *a = thread.pop();
  const ccml::value_t *b = thread.pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int64_t res = (a->v > b->v) ? a->v : b->v;
    thread.push_int(res);
  } else {
    thread.raise_error(thread_error_t::e_bad_argument);
  }
}

void builtin_min(struct ccml::thread_t &thread) {
  const ccml::value_t *a = thread.pop();
  const ccml::value_t *b = thread.pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int64_t res = (a->v < b->v) ? a->v : b->v;
    thread.push_int(res);
  } else {
    thread.raise_error(thread_error_t::e_bad_argument);
  }
}

void builtin_len(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.pop();
  assert(a);
  switch (a->type) {
  case val_type_array: {
    const int64_t res = a->array_size_;
    t.push_int(res);
  } break;
  case val_type_string: {
    assert(a->s);
    const int64_t res = a->s->size();
    t.push_int(res);
  } break;
  default:
    t.raise_error(thread_error_t::e_bad_argument);
    break;
  }
}

void ccml_t::add_builtins_() {
  add_function("abs", builtin_abs, 1);
  add_function("min", builtin_min, 2);
  add_function("max", builtin_max, 2);
  add_function("len", builtin_len, 1);
}

} // namespace ccml
