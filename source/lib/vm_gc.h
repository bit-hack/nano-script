#pragma once
#include <cassert>
#include <string>
#include <unordered_set>
#include <set>
#include <vector>

#include "value.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// ultra simple halt-the-world garbage collector
//
struct value_gc_t {

  value_t *new_int(int32_t value);

  value_t *new_array(int32_t value);

  value_t *new_string(const std::string &value);

  value_t *new_none();

  value_t *copy(const value_t &v);

  void collect(value_t **input, size_t count);

  value_gc_t();
  ~value_gc_t();

protected:
  void release_(value_t *v) {
    if (v) {
      switch (v->type()) {
      case val_type_array:
        // todo: pool arrays?
        assert(v->array_);
        delete[] v->array_;
        v->array_ = nullptr;
        v->array_size_ = 0;
        break;
      case val_type_string:
        string_pool_.push_back(v->s);
        v->s = nullptr;
        break;
      case val_type_none:
        break;
      case val_type_unknown:
        break;
      case val_type_int:
        break;
      default:
        assert(false);
      }
      v->type_ = val_type_unknown;
      avail_.push_back(v);
    }
  }

  void delete_(value_t *v) {
    if (v) {
      if (v->type() == val_type_array) {
        // todo: pool arrays?
        assert(v->array_);
        delete[] v->array_;
        v->array_ = nullptr;
      }
      if (v->type() == val_type_string) {
        delete v->s;
        v->s = nullptr;
      }
      delete v;
    }
  }

  value_t *alloc_() {
    value_t *v = nullptr;
    if (avail_.empty()) {
      // allocate a new value
      v = new value_t;
      assert(v);
      commit_.push_back(v);
    }
    else {
      // pop from front
      v = avail_.back();
      assert(v);
      avail_.pop_back();
    }
    return v;
  }

  void visit_(std::unordered_set<const value_t*> &s, const value_t *v);

  // known dead values
  std::vector<value_t*> avail_;
  // values that have been given out and may be alive
  // std::set<value_t*> allocs_;
  // all values that have been allocated with new
  std::vector<value_t*> commit_;
  // free string pool
  std::vector<std::string*> string_pool_;
};

} // namespce ccml
