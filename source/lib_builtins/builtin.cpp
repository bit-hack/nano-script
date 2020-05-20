#include <math.h>

#include "../lib_compiler/ccml.h"

#include "../lib_vm/thread_error.h"
#include "../lib_vm/vm.h"

#include "builtin.h"


namespace ccml {

void builtin_abs(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *v = t.get_stack().pop();
  assert(v);
  switch (v->type()) {
  case val_type_int: {
    const int32_t res = (v->v < 0) ? (-v->v) : (v->v);
    t.get_stack().push_int(res);
  } break;
  case val_type_float: {
    const float res = (v->f < 0.f) ? (-v->f) : (v->f);
    t.get_stack().push_float(res);
  } break;
  default:
    t.raise_error(thread_error_t::e_bad_argument);
  }
}

// XXX: support variable arg count
void builtin_max(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *a = t.get_stack().pop();
  const ccml::value_t *b = t.get_stack().pop();
  assert(a && b);
  if (a->is_a<val_type_int>() &&
      b->is_a<val_type_int>()) {
    const int32_t ai = a->v;
    const int32_t bi = b->v;
    t.get_stack().push_int(ai > bi ? ai : bi);
    return;
  }
  if (a->is_a<val_type_float>() ||
      b->is_a<val_type_float>()) {
    const float af = a->as_float();
    const float bf = b->as_float();
    t.get_stack().push_float(af > bf ? af : bf);
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

// XXX: support variable arg count
void builtin_min(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *a = t.get_stack().pop();
  const ccml::value_t *b = t.get_stack().pop();
  assert(a && b);
  if (a->is_a<val_type_int>() &&
      b->is_a<val_type_int>()) {
    const int32_t ai = a->v;
    const int32_t bi = b->v;
    t.get_stack().push_int(ai < bi ? ai : bi);
    return;
  }
  if (a->is_a<val_type_float>() ||
      b->is_a<val_type_float>()) {
    const float af = a->as_float();
    const float bf = b->as_float();
    t.get_stack().push_float(af < bf ? af : bf);
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_bitand(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *a = t.get_stack().pop();
  const ccml::value_t *b = t.get_stack().pop();
  assert(a && b);
  if (a->is_a<val_type_int>() &&
      b->is_a<val_type_int>()) {
    const int32_t res = a->v & b->v;
    t.get_stack().push_int(res);
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_len(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *a = t.get_stack().pop();
  assert(a);
  switch (a->type()) {
  case val_type_array: {
    const int32_t res = a->array_size();
    t.get_stack().push_int(res);
  } break;
  case val_type_string: {
    const int32_t res = a->strlen();
    t.get_stack().push_int(res);
  } break;
  default:
    t.raise_error(thread_error_t::e_bad_argument);
    break;
  }
}

void builtin_shl(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *i = t.get_stack().pop();
  const ccml::value_t *v = t.get_stack().pop();
  if (i && v) {
    if (v->is_a<val_type_int>() &&
        i->is_a<val_type_int>()) {
      t.get_stack().push_int(v->v << i->v);
      return;
    }
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_shr(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *i = t.get_stack().pop();
  const ccml::value_t *v = t.get_stack().pop();
  if (i && v) {
    if (v->is_a<val_type_int>() &&
        i->is_a<val_type_int>()) {
      t.get_stack().push_int(uint32_t(v->v) >> i->v);
      return;
    }
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_chr(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *v = t.get_stack().pop();
  if (v && v->is_a<val_type_int>()) {
    char x[2] = {char(v->v), 0};
    t.get_stack().push_string(std::string(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_sin(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(sinf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtin_cos(struct ccml::thread_t &t, int32_t nargs) {
  (void)nargs;
  const ccml::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(cosf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void add_builtins(ccml_t &ccml) {
  ccml.add_function("abs",    builtin_abs,    1);
  ccml.add_function("min",    builtin_min,    2);
  ccml.add_function("max",    builtin_max,    2);
  ccml.add_function("len",    builtin_len,    1);
  ccml.add_function("bitand", builtin_bitand, 2);
  ccml.add_function("sin",    builtin_sin,    1);
  ccml.add_function("cos",    builtin_cos,    1);
#if 0
  ccml.add_function("shl",    builtin_shl,    2);
  ccml.add_function("shr",    builtin_shl,    2);
#endif
  ccml.add_function("chr",    builtin_chr,    1);
}

} // namespace ccml
