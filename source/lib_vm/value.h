#pragma once
#include <assert.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "thread_error.h"


namespace nano {

struct thread_t;
struct value_gc_t;
enum class thread_error_t;

enum value_type_t {
  val_type_unknown,
  val_type_none,
  val_type_int,
  val_type_string,
  val_type_array,
  val_type_float,
  val_type_func,
  val_type_syscall,

  val_type_user = 0x100,
};

struct value_t {

  value_t()
    : type_(val_type_none)
  {}

  value_t(int32_t int_val)
    : type_(val_type_int)
    , v(int_val)
  {}

  template <value_type_t type>
  bool is_a() const {
    return this ? (type_ == type) :
                  (type == val_type_none);
  }

  bool is_number() const {
    return type() == val_type_float ||
           type() == val_type_int;
  }

  void from_int(int32_t val) {
    assert(this);
    type_ = val_type_int;
    v = val;
  }

  int32_t integer() const {
    assert(type() == val_type_int);
    return (int32_t)v;
  }

  const char *string() const {
    assert(type() == val_type_string);
    return (const char *)(this + 1);
  }

  char *string() {
    assert(type() == val_type_string);
    return (char *)(this + 1);
  }

  value_t **array() const {
    assert(type() == val_type_array);
    return (value_t**)(this + 1);
  }

  value_type_t type() const {
    return this == nullptr ?
      value_type_t::val_type_none :
      type_;
  }

  int32_t strlen() const {
    assert(type() == val_type_string);
    return v;
  }

  int32_t array_size() const {
    assert(type() == val_type_array);
    return v;
  }

  float as_float() const {
    assert(this);
    switch (type()) {
    case val_type_int:   return float(v);
    case val_type_float: return float(f);
    default:             assert(false);
    }
    return 0.0f;
  }

  int32_t as_int() const {
    assert(this);
    switch (type()) {
    case val_type_int:   return int32_t(v);
    case val_type_float: return int32_t(f);
    default:             assert(false);
    }
    return 0;
  }

  int32_t as_bool() const {
    switch (type()) {
    case val_type_array:
    case val_type_func:
    case val_type_syscall: return true;
    case val_type_none:    return false;
    case val_type_string:  return v != 0;
    case val_type_float:   return f != 0.f;
    case val_type_int:     return v != 0;
    default:               assert(false);
    }
    return 0;
  }

  std::string to_string() const;

  // note: dont check this directly, please use type() instead as 'this' ptr
  //       can be nullptr
  value_type_t type_;

  union {
    // int value
    // string length if string
    // array length if array
    int32_t v;
    // floating point value
    float f;
  };
};

struct value_stack_t {

  value_stack_t(thread_t &thread, value_gc_t &gc);

  // push a none value
  void push_none();

  // push float number onto the value stack
  void push_float(const float v);

  // push integer onto the value stack
  void push_int(const int32_t v);

  // push string onto the value stack
  void push_string(const std::string &v);

  // push a new function
  void push_func(const int32_t address);

  // push a new syscall
  void push_syscall(const int32_t number);

  void clear() {
    stack_.clear();
  }

  int32_t head() const {
    return int32_t(stack_.size());
  }

  void reserve(uint32_t operand) {
    for (uint32_t i = 0; i < operand; ++i) {
      stack_.emplace_back(nullptr);
    }
  }

  void discard(uint32_t num) {
    for (uint32_t i = 0; i < num; ++i) {
      assert(!stack_.empty());
      stack_.pop_back();
    }
  }

  // peek a stack value
  value_t* peek() {
    assert(!stack_.empty());
    value_t *out = stack_.back();
    return out;
  }

  // pop from the value stack
  value_t* pop() {
    assert(!stack_.empty());
    value_t *out = stack_.back();
    stack_.pop_back();
    return out;
  }

  // push onto the value stack
  void push(value_t *v) {
    stack_.emplace_back(v);
  }

  const value_t *get(const int32_t index) const {
    if (index >= 0 && index < head()) {
      return stack_[index];
    }
    else {
      return nullptr;
    }
  }

  value_t *get(const int32_t index) {
    if (index >= 0 && index < head()) {
      return stack_[index];
    }
    else {
      set_error(thread_error_t::e_bad_getv);
      return nullptr;
    }
  }

  void set(const int32_t index, value_t *val) {
    if (index >= 0 && index < head()) {
      stack_[index] = val;
    }
    else {
      set_error(thread_error_t::e_bad_setv);
    }
  }

  value_t **data() {
    return stack_.data();
  }

  void set_error(thread_error_t error);

protected:
  std::vector<value_t *> stack_;

  struct thread_t &thread_;
  struct value_gc_t &gc_;
};

} // namespace nano
