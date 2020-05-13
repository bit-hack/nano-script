#pragma once

#include <vector>
#include <map>
#include <string>
#include <array>
#include <memory>


#include "common.h"


namespace ccml {

struct line_t {
  int32_t file;
  int32_t line;

  bool operator == (const line_t &rhs) const {
    return file == rhs.file && line == rhs.line;
  }
};

struct program_t {


  // map instruction to line number
  // XXX: this should also account for file path
  typedef std::map<uint32_t, line_t> linetable_t;

  program_t();

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

  program_builder_t &builder() const {
    return *builder_;
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
    return line_t{ -1, -1 };
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

  // add a line to the line table
  void add_line(uint32_t pc, int32_t line) {
    int32_t file = file_table_.size();
    line_table_[pc] = line_t{ file, line};
  }

  // add a file to the file table
  void add_file(std::string &file) {
    file_table_.push_back(file);
  }

  // source files used to compile this program
  std::vector<std::string> file_table_;

  // table of system calls
  std::vector<ccml_syscall_t> syscalls_;

  // function descriptors
  std::vector<function_t> functions_;

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
