#pragma once
#include <assert.h>
#include <stdint.h>

#include <string>
#include <array>

#include "thread_error.h"


namespace ccml {

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
};

struct value_t {

  value_t()
    : type_(val_type_none)
  {}

  value_t(int32_t int_val)
    : type_(val_type_int)
    , v(int_val)
  {}

  bool is_number() const {
    return type() == val_type_float ||
           type() == val_type_int;
  }

  bool is_int() const {
    return type() == val_type_int;
  }

  bool is_string() const {
    return type() == val_type_string;
  }

  bool is_array() const {
    return type() == val_type_array;
  }

  bool is_float() const {
    return type() == val_type_float;
  }

  bool is_none() const {
    return type() == val_type_none;
  }

  void from_int(int32_t val) {
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
    assert(type_ == val_type_string);
    return v;
  }

  int32_t array_size() const {
    assert(type_ == val_type_array);
    return v;
  }

  float as_float() const {
    switch (type_) {
    case val_type_int:   return float(v);
    case val_type_float: return float(f);
    default:             assert(false);
    }
    return 0.0f;
  }

  int32_t as_int() const {
    switch (type_) {
    case val_type_int:   return int32_t(v);
    case val_type_float: return int32_t(f);
    default:             assert(false);
    }
    return 0;
  }

  std::string to_string() const;

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

  static const size_t SIZE = 1024 * 8;

  value_stack_t(thread_t &thread, value_gc_t &gc);

  // push float number onto the value stack
  void push_float(const float v);

  // push integer onto the value stack
  void push_int(const int32_t v);

  // push string onto the value stack
  void push_string(const std::string &v);

  void clear() {
    head_ = 0;
  }

  int32_t head() const {
    return head_;
  }

  void reserve(uint32_t operand) {
    if (head_ + operand >= s_.size()) {
      set_error(thread_error_t::e_stack_overflow);
      return;
    }
    // reserve this many values on the stack
    for (uint32_t i = 0; i < operand; ++i) {
      s_[head_ + i] = nullptr;
    }
    head_ += operand;
  }

  void discard(uint32_t num) {
    assert(head_ >= num);
    head_ -= num;
  }

  // peek a stack value
  value_t* peek() {
    if (head_ <= 0) {
      set_error(thread_error_t::e_stack_underflow);
      return nullptr;
    } else {
      return s_[head_ - 1];
    }
  }

  // pop from the value stack
  value_t* pop() {
    if (head_ <= 0) {
      set_error(thread_error_t::e_stack_underflow);
      return nullptr;
    } else {
      return s_[--head_];
    }
  }

  // push onto the value stack
  void push(value_t *v) {
    if (head_ >= s_.size()) {
      set_error(thread_error_t::e_stack_overflow);
    } else {
      s_[head_++] = v;
    }
  }

  value_t *get(const int32_t index) {
    if (index >= 0 && index < head()) {
      return s_[index];
    }
    else {
      set_error(thread_error_t::e_bad_getv);
      return nullptr;
    }
  }

  void set(const int32_t index, value_t *val) {
    if (index >= 0 && index < head()) {
      s_[index] = val;
    }
    else {
      set_error(thread_error_t::e_bad_setv);
    }
  }

  value_t **data() {
    return s_.data();
  }

  void set_error(thread_error_t error);

protected:
  // value stack
  uint32_t head_;
  std::array<value_t *, SIZE> s_;

  struct thread_t &thread_;
  struct value_gc_t &gc_;
};

} // namespace ccml
