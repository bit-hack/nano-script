#pragma once
#include <cassert>
#include <string>
#include <set>
#include <vector>
#include <array>
//#include <unordered_map>

#include "value.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// simple arena memory allocator
//
struct arena_t {

  arena_t()
    : head_(0)
    , start_(data_.data())
    , end_(data_.data() + data_.size())
  {}

  template <typename type_t>
  type_t *alloc(size_t extra) {
    const size_t size = sizeof(type_t);
    if ((head_ + size + extra) < data_.size()) {
      type_t *out = (type_t*)(data_.data() + head_);
      head_ += size + extra;
      return out;
    }
    return nullptr;
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

  bool owns(const value_t *v) const {
    return v >= start_ && v < end_;
  }

protected:
  size_t head_;
  std::array<uint8_t, 1024 * 1024> data_;

  const void *const start_;
  const void *const end_;
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

  value_t *find_fowards(const value_t *v) {
    for (const auto &p : forward_) {
      if (v == p.first) {
        return p.second;
      }
    }
    return nullptr;
  }

  // since getv puts an array on the stack and then geta/seta to set its member
  // we have to keep a list of already moved arrays.  this could be avoided if
  // we used different instructions to avoid using getv, and instead looked it
  // up when we need.
  //
  // its assumed forward_ will always be very small
  std::vector<std::pair<const value_t *, value_t *>> forward_;

  uint32_t flipflop_;
  std::array<arena_t, 2> space_;
};

} // namespce ccml
