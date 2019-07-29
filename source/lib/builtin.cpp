#include "ccml.h"
#include "vm.h"

// TODO: shl, shr

namespace ccml {

void builtin_abs(struct ccml::thread_t &t) {
  const ccml::value_t *v = t.pop();
  assert(v);
  if (v->is_int()) {
    const int32_t res = (v->v < 0) ? (-v->v) : v->v;
    t.push_int(res);
  } else {
    t.raise_error(thread_error_t::e_bad_argument);
  }
}

void builtin_max(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.pop();
  const ccml::value_t *b = t.pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int32_t res = (a->v > b->v) ? a->v : b->v;
    t.push_int(res);
  } else {
    t.raise_error(thread_error_t::e_bad_argument);
  }
}

void builtin_min(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.pop();
  const ccml::value_t *b = t.pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int32_t res = (a->v < b->v) ? a->v : b->v;
    t.push_int(res);
  } else {
    t.raise_error(thread_error_t::e_bad_argument);
  }
}

void builtin_bitand(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.pop();
  const ccml::value_t *b = t.pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int32_t res = a->v & b->v;
    t.push_int(res);
  } else {
    t.raise_error(thread_error_t::e_bad_argument);
  }
}

void builtin_len(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.pop();
  assert(a);
  switch (a->type()) {
  case val_type_array: {
    const int32_t res = a->array_size();
    t.push_int(res);
  } break;
  case val_type_string: {
    const int32_t res = a->strlen();
    t.push_int(res);
  } break;
  default:
    t.raise_error(thread_error_t::e_bad_argument);
    break;
  }
}

void builtin_shl(struct ccml::thread_t &t) {
  const ccml::value_t *i = t.pop();
  const ccml::value_t *v = t.pop();
  if (i && v) {
    if (v->is_int() && i->is_int()) {
      t.push_int(v->v << i->v);
      return;
    }
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_shr(struct ccml::thread_t &t) {
  const ccml::value_t *i = t.pop();
  const ccml::value_t *v = t.pop();
  if (i && v) {
    if (v->is_int() && i->is_int()) {
      t.push_int(uint32_t(v->v) >> i->v);
      return;
    }
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_chr(struct ccml::thread_t &t) {
  const ccml::value_t *v = t.pop();
  if (v && v->is_int()) {
    char x[2] = {char(v->v), 0};
    t.push_string(std::string(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void ccml_t::add_builtins_() {
  add_function("abs", builtin_abs, 1);
  add_function("min", builtin_min, 2);
  add_function("max", builtin_max, 2);
  add_function("len", builtin_len, 1);
  add_function("bitand", builtin_bitand, 2);
#if 0
  add_function("shl", builtin_shl, 2);
  add_function("shr", builtin_shl, 2);
#endif
  add_function("chr", builtin_chr, 1);
}

} // namespace ccml
