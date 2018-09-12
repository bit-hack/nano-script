#include "ccml.h"
#include "vm.h"

void builtin_abs(struct ccml::thread_t &thread) {
  const ccml::value_t v = thread.pop();
  thread.push(v < 0 ? -v : v);
}

void builtin_max(struct ccml::thread_t &thread) {
  const ccml::value_t a = thread.pop();
  const ccml::value_t b = thread.pop();
  thread.push(a > b ? a : b);
}

void builtin_min(struct ccml::thread_t &thread) {
  const ccml::value_t a = thread.pop();
  const ccml::value_t b = thread.pop();
  thread.push(a < b ? a : b);
}
