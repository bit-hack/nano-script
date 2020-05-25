#include <climits>
#include <cstring>

#include "program.h"
#include "instructions.h"

#include "vm.h"
#include "thread.h"

using namespace nano;

namespace nano {

vm_t::vm_t(program_t &program)
  : program_(program)
  , gc_(new value_gc_t) {}

void vm_t::gc_collect() {
  // traverse globals
  gc_->trace(g_.data(), g_.size());
  // traverse thread stack
  for (thread_t *t : threads_) {
    gc_->trace(t->stack_.data(), t->stack_.head());
  }
  // collect
  gc_->collect();
}

void vm_t::reset() {
  // reset the garbage collector
  gc_->reset();
  // delete all threads
  for (thread_t *t : threads_) {
    delete t;
  }
  threads_.clear();
}

bool vm_t::call_init() {
  thread_t thread(*this);
  return thread.call_init_();
}

bool vm_t::call_once(const function_t &func,
                     int32_t argc,
                     const value_t *argv,
                     value_t* &return_code,
                     thread_error_t &error) {

  error = nano::thread_error_t::e_success;
  return_code = nullptr;

  thread_t thread(*this);
  gc_collect();
  if (!thread.prepare(func, argc, argv)) {
    error = thread_error_t::e_bad_prepare;
    return false;
  }
  while (!thread.finished() && !thread.has_error()) {
    if (!thread.resume(128 * 1024)) {
      break;
    }
  }
  if (!thread.finished()) {
    // something went wrong
    return false;
  }
  if (thread.has_error()) {
    error = thread.get_error();
    return false;
  }
  return_code = thread.get_return_code();
  return true;
}

} // namespace nano
