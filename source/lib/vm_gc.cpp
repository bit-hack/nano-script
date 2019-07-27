#include <string.h>

#include "vm_gc.h"

namespace ccml {

value_t *value_gc_t::new_int(int32_t value) {
  value_t *v = space_to().alloc<value_t>(0);
  assert(v);
  v->type_ = val_type_int;
  v->v = value;
  return v;
}

value_t *value_gc_t::new_array(int32_t value) {
  size_t size = value * sizeof(value_t *);
  value_t *v = space_to().alloc<value_t>(size);
  assert(v);
  v->type_ = val_type_array;
  assert(value > 0);
  memset(v->array(), 0, size);
  v->v = int32_t(value);
  return v;
}

value_t *value_gc_t::new_string(const std::string &value) {
  const size_t size = value.size();
  value_t *v = space_to().alloc<value_t>(value.size() + 1);
  assert(v);
  v->type_ = val_type_string;
  v->v = size;
  // copy string
  memcpy((char *)(v + 1), value.data(), size);
  // insert '\0' terminator
  v->string()[size] = 0;
  return v;
}

value_t *value_gc_t::new_string(int32_t len) {
  value_t *v = space_to().alloc<value_t>(len + 1);
  assert(v);
  v->type_ = val_type_string;
  v->v = len;
  v->string()[0] = '\0';
  return v;
}

value_t *value_gc_t::new_none() {
  return nullptr;
}

value_t *value_gc_t::copy(const value_t &a) {
  switch (a.type()) {
  case val_type_int:
    return new_int(a.v);
  case val_type_string: {
    value_t *v = new_string(a.strlen());
    memcpy(v->string(), a.string(), a.strlen());
    v->string()[a.strlen()] = 0;
    return v;
  }
  case val_type_none:
    return new_none();
  default:
    assert(false);
    return nullptr;
  }
}

void value_gc_t::collect(value_t **list, size_t num) {

  // currently the grammar doesnt allow us to have cyclic data structures
  // but be aware we might need to support it when the language evolves.

  const int before = space_to().size();
  swap();
  space_to().clear();
  collect_imp_(list, num);
  const int after = space_to().size();
  //  printf("%5d %5d\n", before, after);
}

void value_gc_t::collect_imp_(value_t **list, size_t num) {

  const arena_t &from = space_from();
  arena_t &to = space_to();

  for (size_t i = 0; i < num; ++i) {
    value_t *v = list[i];
    switch (v->type()) {
    case val_type_none: {
      break;
    }
    case val_type_int: {
      value_t *n = to.alloc<value_t>(0);
      n->type_ = val_type_int;
      n->v = v->integer();
      list[i] = n;
      break;
    }
    case val_type_string: {
      assert(strlen(v->string()) == v->v);
      const size_t size = v->v;
      value_t *n = to.alloc<value_t>(size + 1);
      n->type_ = val_type_string;
      n->v = size;
      memcpy(n->string(), v->string(), size + 1);
      list[i] = n;
      break;
    }
    case val_type_array: {
      // collect child elements
      const size_t size = v->array_size();
      collect_imp_(v->array(), size);
      value_t *n = to.alloc<value_t>(size * sizeof(value_t *));
      n->type_ = val_type_array;
      n->v = size;
      memcpy(n->array(), v->array(), size * sizeof(value_t *));
      list[i] = n;
      break;
    }
    default:
      assert(!"unknown type");
    }
  }
}

} // namespace ccml
