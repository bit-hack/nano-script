#include <string.h>

#include "vm_gc.h"

namespace ccml {

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

  const size_t reserve = commit_.size() / 2;

  std::unordered_set<const value_t *> alive(reserve);
  for (size_t i = 0; i < num; ++i) {
    value_t *x = v[i];
    if (x) {
      visit_(alive, x);
    }
  }
  avail_.clear();
  for (auto itt = commit_.begin(); itt != commit_.end(); ++itt) {
    if (alive.count(*itt) == 0) {
      value_t *val = *itt;
      assert(val);
      release_(val);
    }
  }
}

} // namespace ccml
