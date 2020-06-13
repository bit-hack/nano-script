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

vm_t::~vm_t() {
  reset();
}

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
  function_t *init = program_.function_find("@init");
  if (!init) {
    return false;
  }
  thread_t *t = new_thread(*init, 0, nullptr);
  if (!t) {
    return false;
  }
  return resume(1024 * 8);
}

bool vm_t::call_once(const function_t &func,
                     int32_t argc,
                     const value_t *argv,
                     value_t* &return_code,
                     thread_error_t &error) {

  error = nano::thread_error_t::e_success;
  return_code = nullptr;

  thread_t *thread = new_thread(func, argc, argv);
  gc_collect();
  while (!thread->finished() && !thread->has_error()) {
    if (!thread->resume(128 * 1024)) {
      break;
    }
  }
  if (thread->has_error()) {
    error = thread->get_error();
    return false;
  }
  return_code = thread->get_return_value();
  return true;
}

thread_t *vm_t::new_thread(const function_t &func,
                           int32_t argc,
                           const value_t *argv) {
  std::unique_ptr<thread_t> t(new thread_t(*this));
  if (!t->prepare(func, argc, argv)) {
    return nullptr;
  }
  thread_t *inst = t.get();
  // insert into the thread list
  threads_.push_front(t.release());
  return inst;
}

bool vm_t::resume(uint32_t cycles) {
  auto itt = threads_.begin();
  for (; itt != threads_.end();) {
    thread_t *t = *itt;
    assert(t);
    if (t->finished() || t->has_error()) {
      // delete the thread
      delete t;
      itt = threads_.erase(itt);
      continue;
    }
    // we will be advancing regardless
    ++itt;
    if (!t->resume(cycles)) {
      // thread has an error
      assert(t->has_error());
      return false;
    }
  }
  return true;
}

bool vm_t::has_error() const {
  return false;
}

bool vm_t::finished() const {
  for (const auto &t : threads_) {
    if (!t->finished()) {
      return false;
    }
  }
  return true;
}

} // namespace nano
