#pragma once
#include <cassert>
#include <string>
#include <unordered_set>
#include <set>
#include <vector>
#include <array>

#include "value.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// simple arena memory allocator
//
struct arena_t {

  arena_t()
    : head_(0) 
  {}

  template <typename type_t>
  type_t *alloc(size_t extra) {
    size_t size = sizeof(type_t);
    assert(head_ + size < data_.size());
    type_t *out = (type_t*)(data_.data() + head_);
    head_ += size + extra;
    return out;
  }

  void clear() {
    head_ = 0;
  }

  size_t size() const {
    return head_;
  }

  size_t capacity() const {
    return data_.size();
  }

protected:
  size_t head_;
  std::array<uint8_t, 1024 * 1024> data_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// ultra simple halt-the-world garbage collector
//
struct value_gc_t {

  value_t *new_int(int32_t value);

  value_t *new_array(int32_t value);

  value_t *new_string(const std::string &value);

  value_t *new_string(int32_t length);

  value_t *new_none();

  value_t *copy(const value_t &v);

  void collect();

  void trace(value_t **input, size_t count);

  value_gc_t()
    : flipflop_(0)
  {}

protected:

  arena_t &space_from() {
    return space_[flipflop_ & 1];
  }

  arena_t &space_to() {
    return space_[(flipflop_ & 1) ^ 1];
  }

  void swap() {
    flipflop_ ^= 1;
  }

  uint32_t flipflop_;
  std::array<arena_t, 2> space_;
};

} // namespce ccml
