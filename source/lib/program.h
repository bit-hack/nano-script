#pragma once

#include <vector>
#include <map>
#include <string>
#include <array>
#include <memory>


#include "common.h"


namespace ccml {

struct program_t {

  // map instruction to line number
  // XXX: this should also account for file path
  typedef std::map<uint32_t, uint32_t> linetable_t;

  program_t();

  const uint8_t *data() const {
    return code_.data();
  }

  size_t size() const {
    return code_.size();
  }

  const uint8_t *end() const {
    return code_.data() + code_.size();
  }

  program_builder_t &builder() const {
    return *builder_;
  }

  const linetable_t &line_table() const {
    return line_table_;
  }

  int32_t get_line(uint32_t pc) const {
    auto itt = line_table_.find(pc);
    if (itt != line_table_.end()) {
      return itt->second;
    }
    return -1;
  }

  void reset();

  const std::vector<std::string> &strings() const {
    return strings_;
  }

  std::vector<std::string> &strings() {
    return strings_;
  }

  const std::vector<function_t> &functions() const {
    return functions_;
  }

  std::vector<function_t> &functions() {
    return functions_;
  }

  const std::vector<ccml_syscall_t> &syscall() const {
    return syscalls_;
  }

  const function_t *function_find(const std::string &name) const;
        function_t *function_find(const std::string &name);
        function_t *function_find(uint32_t pc);

protected:
  friend struct program_builder_t;

  void add_line(uint32_t pc, uint32_t line) {
    line_table_[pc] = line;
  }

  struct pc_range_t {
    uint32_t pc_start_;
    uint32_t pc_end_;
    ast_decl_func_t *f_;  // XXX: should not be any AST refs here
  };

  // table of system calls
  std::vector<ccml_syscall_t> syscalls_;

  // function descriptors
  std::vector<function_t> functions_;

  // pc to function mapping
  std::vector<pc_range_t> pc_range_;

  // bytecode array
  std::vector<uint8_t> code_;

  // program builder
  std::unique_ptr<program_builder_t> builder_;

  // line table [PC -> Line]
  linetable_t line_table_;

  // string table
  std::vector<std::string> strings_;
};

} // namespace ccml
