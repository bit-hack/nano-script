#pragma once
#include <stdint.h>

namespace ccml {

struct value_t {

  static const int32_t FBITS = 12;
  static const int64_t FMASK = (1 << FBITS) - 1;

  int64_t v;
};

inline int32_t value_to_int(const value_t &a) {
  return int32_t(a.v);
}

inline value_t value_from_int(const int32_t &a) {
  return value_t{a};
}

inline value_t operator&&(const value_t &a, const value_t &b) {
  return value_t{a.v && b.v};
}

inline value_t operator||(const value_t &a, const value_t &b) {
  return value_t{a.v || b.v};
}

inline value_t operator+(const value_t &a, const value_t &b) {
  return value_t{a.v + b.v};
}

inline value_t operator-(const value_t &a, const value_t &b) {
  return value_t{a.v - b.v};
}

inline value_t operator*(const value_t &a, const value_t &b) {
  return value_t{a.v * b.v};
}

inline bool operator<(const value_t &a, const value_t &b) {
  return a.v < b.v;
}

inline bool operator<=(const value_t &a, const value_t &b) {
  return a.v <= b.v;
}

inline bool operator>(const value_t &a, const value_t &b) {
  return a.v > b.v;
}

inline bool operator>=(const value_t &a, const value_t &b) {
  return a.v >= b.v;
}

inline bool operator==(const value_t &a, const value_t &b) {
  return a.v == b.v;
}

inline bool operator!=(const value_t &a, const value_t &b) {
  return a.v != b.v;
}

inline bool value_div(const value_t &a, const value_t &b, value_t &r) {
  if (b.v == 0) {
    return false;
  } else {
    r = value_t{a.v / b.v};
    return true;
  }
}

inline bool value_mod(const value_t &a, const value_t &b, value_t &r) {
  if (b.v == 0) {
    return false;
  } else {
    r = value_t{a.v % b.v};
    return true;
  }
}

inline value_t operator-(const value_t &a) {
  return value_t{-a.v};
}

inline value_t operator!(const value_t &a) {
  return value_t{!a.v};
}

} // namespace ccml
