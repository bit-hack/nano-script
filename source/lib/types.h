#pragma once
#include <cstdint>
#include <string>
#include <vector>


namespace ccml {

// system call interface function
typedef void(*ccml_syscall_t)(struct thread_t &thread, int32_t num_args);

struct line_t {
  int32_t file;
  int32_t line;

  bool operator == (const line_t &rhs) const {
    return file == rhs.file && line == rhs.line;
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

  // syscall callback or nullptr
  ccml_syscall_t sys_;

  // number of arguments syscall takes
  uint32_t sys_num_args_;

  // code address range
  uint32_t code_start_, code_end_;

  // function locals
  std::vector<identifier_t> locals_;

  // function arguments
  std::vector<identifier_t> args_;

  uint32_t num_args() const {
    return is_syscall() ? sys_num_args_ : args_.size();
  }

  bool is_syscall() const {
    return sys_ != nullptr;
  }

  // generic constructor
  function_t()
    : sys_(nullptr)
    , code_start_(0)
    , code_end_(0) {}

  // syscall constructor
  function_t(const std::string &name, ccml_syscall_t sys, int32_t num_args)
    : name_(name)
    , sys_(sys)
    , sys_num_args_(num_args) {}

  // compiled function constructor
  function_t(const std::string &name, int32_t pos)
    : name_(name)
    , sys_(nullptr)
    , code_start_(pos)
    , code_end_(pos) {}
};

} // namespace ccml
