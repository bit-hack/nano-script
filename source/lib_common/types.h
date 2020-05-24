#pragma once
#include <cstdint>
#include <string>
#include <vector>


namespace nano {

// system call interface function
typedef void(*nano_syscall_t)(struct thread_t &thread, int32_t num_args);

struct line_t {

  int32_t file;
  int32_t line;

  line_t() : file(-1), line(-1) {}
  line_t(int32_t f, int32_t l): file(f), line(l) {}

  bool operator == (const line_t &rhs) const {
    return file == rhs.file && line == rhs.line;
  }

  bool operator != (const line_t &rhs) const {
    return file != rhs.file || line != rhs.line;
  }

  bool operator < (const line_t &rhs) const {
    if (file < rhs.file) {
      return true;
    }
    if (file == rhs.file) {
      return line < rhs.line;
    }
    return false;
  }
};

struct identifier_t {
  // identifier name
  std::string name_;
  // stack offset
  int32_t offset_;
};

struct function_t {

  const std::string &name() const {
    return name_;
  }

  // name of the function
  std::string name_;

  // code address range
  int32_t code_start_, code_end_;

  // function locals
  std::vector<identifier_t> locals_;

  // function arguments
  std::vector<identifier_t> args_;

  // number of function arguments present
  int32_t num_args() const {
    return int32_t(args_.size());
  }

  // generic constructor
  function_t()
    : code_start_(0)
    , code_end_(0) {}

  function_t(const std::string &name, int32_t pos)
    : name_(name)
    , code_start_(0)
    , code_end_(0)
  {}
};

} // namespace nano
