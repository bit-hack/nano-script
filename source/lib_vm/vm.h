#pragma once
#include <vector>
#include <array>
#include <bitset>
#include <string>
#include <set>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <list>

#include "../lib_common/common.h"
#include "../lib_common/types.h"

#include "vm_gc.h"


namespace nano {

struct thread_t;

struct handlers_t {

  handlers_t()
    : on_thread_error(nullptr)
    , on_thread_finish(nullptr)
    , on_member_get(nullptr)
    , on_member_set(nullptr)
    , on_array_get(nullptr)
    , on_array_set(nullptr)
    , on_equals(nullptr)
    , on_add(nullptr)
    , on_sub(nullptr)
    , on_mul(nullptr)
    , on_div(nullptr)
  {}

  bool(*on_thread_error)(thread_t &t);

  bool(*on_thread_finish)(thread_t &t);

  bool (*on_member_get)(thread_t &t, value_t *v, const std::string &member);

  bool (*on_member_set)(thread_t &t, value_t *obj, value_t *expr, const std::string &member);

  bool (*on_array_get)(thread_t &t, value_t *a, const value_t *index);

  bool (*on_array_set)(thread_t &t, value_t *a, const value_t *index, value_t *val);

  bool (*on_equals)(thread_t &t, const value_t *l, const value_t *r);

  bool (*on_add)(thread_t &t, const value_t *l, const value_t *r);

  bool (*on_sub)(thread_t &t, const value_t *l, const value_t *r);

  bool (*on_mul)(thread_t &t, const value_t *l, const value_t *r);

  bool (*on_div)(thread_t &t, const value_t *l, const value_t *r);
};

struct vm_t {

  vm_t(program_t &program);
  ~vm_t();

  void reset();

  // call the init function
  bool call_init();

  // execute a single function
  // XXX: this return value could die and is not safe
  // XXX: remove return value and error field
  bool call_once(const function_t &func,
                 int32_t argc,
                 const value_t **argv,
                 value_t* &return_code,
                 thread_error_t &error);

  // create a new thread
  // note: returned pointer is owned by the vm_t do not delete it
  thread_t* new_thread(const function_t &func,
                       int32_t argc,
                       const value_t **argv);

  // resume the VM for a number of cycles
  bool resume(uint32_t cycles);

  // return true if any of the thread raised an error
  bool has_error() const;

  // return true if all threads have finished
  bool finished() const;

  // return the thread list
  const std::list<thread_t *> &thread() const {
    return threads_;
  }

  // return the list of globals
  const std::vector<value_t *> &globals() const {
    return g_;
  }

  // return the bound program
  const program_t &program() const {
    return program_;
  }

  // handlers
  handlers_t handlers;

protected:
  friend struct thread_t;

  // the currently bound program
  program_t &program_;

  // garbage collector
  std::unique_ptr<value_gc_t> gc_;
  void gc_collect();

  // globals
  std::vector<value_t*> g_;

  // threads
  std::list<thread_t *> threads_;
};

} // namespace nano
