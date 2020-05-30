#include <math.h>

#include "../lib_compiler/nano.h"

#include "../lib_vm/thread_error.h"
#include "../lib_vm/vm.h"
#include "../lib_vm/thread.h"

#include "builtin.h"


namespace nano {

static void builtin_abs(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
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

static void builtin_max(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *a = t.get_stack().pop();
  const nano::value_t *b = t.get_stack().pop();
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

static void builtin_min(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *a = t.get_stack().pop();
  const nano::value_t *b = t.get_stack().pop();
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

static void builtin_bitand(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *a = t.get_stack().pop();
  const nano::value_t *b = t.get_stack().pop();
  assert(a && b);
  if (a->is_a<val_type_int>() &&
      b->is_a<val_type_int>()) {
    const int32_t res = a->v & b->v;
    t.get_stack().push_int(res);
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

static void builtin_len(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *a = t.get_stack().pop();
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

static void builtin_chr(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
  if (v && v->is_a<val_type_int>()) {
    char x[2] = {char(v->v), 0};
    t.get_stack().push_string(std::string(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

static void builtin_sin(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(sinf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

static void builtin_cos(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(cosf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

static void builtin_tan(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(tanf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

static void builtin_round(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(roundf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

static void builtin_floor(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(floorf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

static void builtin_ceil(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(ceilf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

static void builtin_sqrt(struct nano::thread_t &t, int32_t nargs) {
  (void)nargs;
  const nano::value_t *v = t.get_stack().pop();
  if (v && (v->is_number())) {
    const float x = v->as_float();
    t.get_stack().push_float(sqrtf(x));
    return;
  }
  t.raise_error(thread_error_t::e_bad_argument);
}

void builtins_register(nano_t &nano) {

  nano.syscall_register("abs", 1);
  nano.syscall_register("min", 2);
  nano.syscall_register("max", 2);

  nano.syscall_register("len", 1);

  nano.syscall_register("bitand", 2);

  nano.syscall_register("sin", 1);
  nano.syscall_register("cos", 1);
  nano.syscall_register("tan", 1);

  nano.syscall_register("chr", 1);

  nano.syscall_register("round", 1);
  nano.syscall_register("ceil",  1);
  nano.syscall_register("floor", 1);

  nano.syscall_register("sqrt", 1);
}

void builtins_resolve(program_t &prog) {

  std::map<std::string, nano_syscall_t> map;
  map["abs"]    = builtin_abs;
  map["min"]    = builtin_min;
  map["max"]    = builtin_max;
  map["len"]    = builtin_len;
  map["bitand"] = builtin_bitand;

  map["sin"]    = builtin_sin;
  map["cos"]    = builtin_cos;
  map["tan"]    = builtin_tan;

  map["chr"]    = builtin_chr;

  map["round"] = builtin_round;
  map["ceil"]  = builtin_ceil;
  map["floor"] = builtin_floor;

  map["sqrt"]  = builtin_sqrt;

  for (auto &itt : prog.syscalls()) {
    auto i = map.find(itt.name_);
    if (i != map.end()) {
      itt.call_ = i->second;
    }
  }
}

} // namespace nano
