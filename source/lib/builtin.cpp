#include "ccml.h"
#include "vm.h"

// TODO: shl, shr

namespace ccml {

void builtin_abs(struct ccml::thread_t &thread) {
  const ccml::value_t *v = thread.pop();
  assert(v);
  if (v->is_int()) {
    const int64_t res = (v->v < 0) ? (-v->v) : v->v;
    thread.push(thread.gc().new_int(res));
  }
  else {
    assert(false);
    // raise error
    // thread.raise_error(...);
  }
}

void builtin_max(struct ccml::thread_t &thread) {
  const ccml::value_t *a = thread.pop();
  const ccml::value_t *b = thread.pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int64_t res = (a->v > b->v) ? a->v : b->v;
    thread.push(thread.gc().new_int(res));
  }
  else {
    assert(false);
    // raise error
    // thread.raise_error(...);
  }
}

void builtin_min(struct ccml::thread_t &thread) {
  const ccml::value_t *a = thread.pop();
  const ccml::value_t *b = thread.pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int64_t res = (a->v < b->v) ? a->v : b->v;
    thread.push(thread.gc().new_int(res));
  }
  else {
    assert(false);
    // raise error
    // thread.raise_error(...);
  }
}

void builtin_len(struct ccml::thread_t &thread) {
  const ccml::value_t *a = thread.pop();
  assert(a);
  if (a->is_array()) {
    const int64_t res = a->array_size_;
    thread.push(thread.gc().new_int(res));
  }
  else {
    assert(false);
    // raise error
    // thread.raise_error(...);
  }
}

void ccml_t::add_builtins_() {
  add_function("abs", builtin_abs, 1);
  add_function("min", builtin_min, 2);
  add_function("max", builtin_max, 2);
  add_function("len", builtin_len, 1);
}

} // namespace
