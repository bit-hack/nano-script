#pragma once

#include <vector>
#include <map>
#include <string>
#include <array>
#include <memory>
#include <cassert>


#include "common.h"
#include "types.h"


namespace nano {

struct program_t {

  // map instruction to line number
  typedef std::map<int32_t, line_t> linetable_t;

  // entry in the syscall table
  struct syscall_entry_t {
    std::string name_;
    ccml_syscall_t call_;
  };

  // access the raw opcodes
  const uint8_t *data() const {
    return code_.data();
  }

  // size of the raw opcodes
  size_t size() const {
    return code_.size();
  }

  const uint8_t *end() const {
    return code_.data() + code_.size();
  }

  const linetable_t &line_table() const {
    return line_table_;
  }

  line_t get_line(uint32_t pc) const {
    auto itt = line_table_.find(pc);
    if (itt != line_table_.end()) {
      return itt->second;
    }
    // no line found
    return line_t{};
  }

  void reset();

  const std::vector<std::string> &strings() const {
    return strings_;
  }

  std::vector<std::string> &strings() {
    return strings_;
  }

  const std::vector<identifier_t> &globals() const {
    return globals_;
  }

  const std::vector<function_t> &functions() const {
    return functions_;
  }

  std::vector<function_t> &functions() {
    return functions_;
  }

  std::vector<syscall_entry_t> &syscalls() {
    return syscalls_;
  }

  bool syscall_resolve(const std::string &name, ccml_syscall_t syscall);

  const function_t *function_find(const std::string &name) const;
        function_t *function_find(const std::string &name);
        function_t *function_find(int32_t pc);

  // serialization functions
  bool serial_save(const char *path);
  bool serial_load(const char *path);

protected:
  friend struct program_builder_t;

  // XXX: add filetable

  // add a line to the line table
  void add_line(int32_t pc, line_t line) {
    line_table_[pc] = line;
  }

  // global variables
  std::vector<identifier_t> globals_;

  // table of system calls
  std::vector<syscall_entry_t> syscalls_;

  // function descriptors
  std::vector<function_t> functions_;

  // bytecode array
  std::vector<uint8_t> code_;

  // line table [PC -> Line]
  linetable_t line_table_;

  // string table
  std::vector<std::string> strings_;
};

} // namespace nano
