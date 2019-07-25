#include <string.h>

#include "vm_gc.h"

namespace ccml {

value_gc_t::value_gc_t() {
#if 1
  for (int i = 0; i < 128; ++i) {
    value_t *v = new value_t;
    commit_.push_back(v);
    avail_.push_back(v);
  }
#endif
}

value_gc_t::~value_gc_t() {
  for (value_t *v : commit_) {
    delete_(v);
  }
  commit_.clear();
  for (std::string *s : string_pool_) {
    delete s;
  }
  string_pool_.clear();
}

value_t *value_gc_t::new_int(int32_t value) {
  value_t *v = alloc_();
  v->type = val_type_int;
  v->v = value;
  return v;
}

value_t *value_gc_t::new_array(int32_t value) {
  value_t *v = alloc_();
  v->type = val_type_array;
  assert(value > 0);
  // todo: pool these
  v->array_ = new value_t *[size_t(value)];
  assert(v->array_);
  memset(v->array_, 0, size_t(sizeof(value_t *) * value));
  v->array_size_ = int32_t(value);
  return v;
}

value_t *value_gc_t::new_string(const std::string &value) {
  value_t *v = alloc_();
  v->type = val_type_string;
  if (string_pool_.empty()) {
    v->s = new std::string(value);
  }
  else {
    v->s = string_pool_.back();
    string_pool_.pop_back();
    *(v->s) = value;
  }
  return v;
}

value_t *value_gc_t::new_none() {
  value_t *v = alloc_();
  v->type = val_type_none;
  return v;
}

value_t *value_gc_t::copy(const value_t &a) {
  switch (a.type) {
  case val_type_int:
    return new_int(a.v);
  case val_type_string:
    return new_string(a.str());
  case val_type_none:
    return new_none();
  default:
    assert(false);
    return nullptr;
  }
}

void value_gc_t::visit_(std::unordered_set<const value_t *> &s, const value_t *v) {
  if (v == nullptr) {
    return;
  }
  if (v->is_array()) {
    for (int i = 0; i < v->array_size_; ++i) {
      visit_(s, v->array_[i]);
    }
  }
  s.insert(v);
}

void value_gc_t::collect(value_t **v, size_t num) {

  // currently the grammar doesnt allow us to have cyclic data structures
  // but be aware we might need to support it when the language evolves.

  // we should collect stats about how many values are alive vs commited
  // for each collect itteration.
  const size_t reserve = commit_.size() / 2;

  // we could convert this to be a marking bitmap if we reserve all out
  // value_t's from a contiguous chunk of memory.
  std::unordered_set<const value_t *> alive(reserve);
  for (size_t i = 0; i < num; ++i) {
    value_t *x = v[i];
    if (x) {
      visit_(alive, x);
    }
  }

  // clear available nodes
  avail_.clear();
  
  // reclaim from commit list
  for (auto itt = commit_.begin(); itt != commit_.end(); ++itt) {
    if (alive.count(*itt) == 0) {
      value_t *val = *itt;
      assert(val);
      release_(val);
    }
  }
}

} // namespace ccml
