#include "ccml.h"
#include "vm.h"

namespace ccml {

void builtin_abs(struct ccml::thread_t &t) {
  const ccml::value_t *v = t.stack().pop();
  assert(v);
  switch (v->type()) {
  case val_type_int: {
    const int32_t res = (v->v < 0) ? (-v->v) : (v->v);
    t.stack().push_int(res);
  } break;
  case val_type_float: {
    const float res = (v->f < 0.f) ? (-v->f) : (v->f);
    t.stack().push_float(res);
  } break;
  default:
    t.raise_error(thread_error_t::e_bad_argument);
  }
}

void builtin_max(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.stack().pop();
  const ccml::value_t *b = t.stack().pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int32_t ai = a->v;
    const int32_t bi = b->v;
    t.stack().push_int(ai > bi ? ai : bi);
    return;
  }
  if (a->is_float() || b->is_float()) {
    const float af = a->as_float();
    const float bf = b->as_float();
    t.stack().push_float(af > bf ? af : bf);
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_min(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.stack().pop();
  const ccml::value_t *b = t.stack().pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int32_t ai = a->v;
    const int32_t bi = b->v;
    t.stack().push_int(ai < bi ? ai : bi);
    return;
  }
  if (a->is_float() || b->is_float()) {
    const float af = a->as_float();
    const float bf = b->as_float();
    t.stack().push_float(af < bf ? af : bf);
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_bitand(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.stack().pop();
  const ccml::value_t *b = t.stack().pop();
  assert(a && b);
  if (a->is_int() && b->is_int()) {
    const int32_t res = a->v & b->v;
    t.stack().push_int(res);
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_len(struct ccml::thread_t &t) {
  const ccml::value_t *a = t.stack().pop();
  assert(a);
  switch (a->type()) {
  case val_type_array: {
    const int32_t res = a->array_size();
    t.stack().push_int(res);
  } break;
  case val_type_string: {
    const int32_t res = a->strlen();
    t.stack().push_int(res);
  } break;
  default:
    t.raise_error(thread_error_t::e_bad_argument);
    break;
  }
}

void builtin_shl(struct ccml::thread_t &t) {
  const ccml::value_t *i = t.stack().pop();
  const ccml::value_t *v = t.stack().pop();
  if (i && v) {
    if (v->is_int() && i->is_int()) {
      t.stack().push_int(v->v << i->v);
      return;
    }
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_shr(struct ccml::thread_t &t) {
  const ccml::value_t *i = t.stack().pop();
  const ccml::value_t *v = t.stack().pop();
  if (i && v) {
    if (v->is_int() && i->is_int()) {
      t.stack().push_int(uint32_t(v->v) >> i->v);
      return;
    }
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_chr(struct ccml::thread_t &t) {
  const ccml::value_t *v = t.stack().pop();
  if (v && v->is_int()) {
    char x[2] = {char(v->v), 0};
    t.stack().push_string(std::string(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_sin(struct ccml::thread_t &t) {
  const ccml::value_t *v = t.stack().pop();
  if (v && (v->is_int() || v->is_float())) {
    const float x = v->as_float();
    t.stack().push_float(sinf(x));
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_cos(struct ccml::thread_t &t) {
  const ccml::value_t *v = t.stack().pop();
  if (v && (v->is_int() || v->is_float())) {
    const float x = v->as_float();
    t.stack().push_float(cosf(x));
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void ccml_t::add_builtins_() {
  add_function("abs", builtin_abs, 1);
  add_function("min", builtin_min, 2);
  add_function("max", builtin_max, 2);
  add_function("len", builtin_len, 1);
  add_function("bitand", builtin_bitand, 2);
  add_function("sin", builtin_sin, 1);
  add_function("cos", builtin_cos, 1);
#if 0
  add_function("shl", builtin_shl, 2);
  add_function("shr", builtin_shl, 2);
#endif
  add_function("chr", builtin_chr, 1);
}

} // namespace ccml
