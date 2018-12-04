#include "ccml.h"
#include "vm.h"

// TODO: shl, shr

namespace ccml {

void builtin_abs(struct ccml::thread_t &thread) {
  const ccml::value_t v = thread.pop();
#if USE_OLD_VALUE_TYPE
  thread.push(v < 0 ? -v : v);
#else
  thread.push(value_lt(v, value_t{0}) ? value_neg(v) : v);
#endif
}

void builtin_max(struct ccml::thread_t &thread) {
  const ccml::value_t a = thread.pop();
  const ccml::value_t b = thread.pop();
#if USE_OLD_VALUE_TYPE
  thread.push(a > b ? a : b);
#else
  thread.push(value_gt(a, b) ? a : b);
#endif
}

void builtin_min(struct ccml::thread_t &thread) {
  const ccml::value_t a = thread.pop();
  const ccml::value_t b = thread.pop();
#if USE_OLD_VALUE_TYPE
  thread.push(a < b ? a : b);
#else
  thread.push(value_lt(a, b) ? a : b);
#endif
}

void ccml_t::add_builtins_() {
  add_function("abs", builtin_abs, 1);
  add_function("min", builtin_min, 2);
  add_function("max", builtin_max, 2);
}

} // namespace
