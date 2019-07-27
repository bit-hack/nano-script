#pragma once
#include <stdint.h>

namespace ccml {

enum value_type_t {
  val_type_unknown,
  val_type_none,
  val_type_int,
  val_type_string,
  val_type_array,
};

struct value_t {

  value_t()
    : type_(val_type_none)
  {}

  value_t(int32_t int_val)
    : type_(val_type_int)
    , v(int_val)
  {}

  bool is_int() const {
    return type() == val_type_int;
  }

  bool is_string() const {
    return type() == val_type_string;
  }

  bool is_array() const {
    return type() == val_type_array;
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

  const value_type_t type() const {
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

  value_type_t type_;

  // int value
  // string length if string
  // array length if array
  int32_t v;
};

} // namespace ccml
