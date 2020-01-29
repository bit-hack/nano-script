#pragma once

#include <vector>
#include <map>
#include <string>
#include <array>
#include <memory>


#include "common.h"

namespace ccml {

struct program_t {

  program_t();

  const uint8_t *data() const {
    return code_.data();
  }

  uint8_t *data() {
    return code_.data();
  }

  const size_t size() const {
    return code_.size();
  }

  const uint8_t *end() const {
    return code_.data() + code_.size();
  }

  asm_stream_t &stream() const {
    return *stream_;
  }

  const std::map<uint32_t, uint32_t> &line_table() const {
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

protected:
  friend struct asm_stream_t;

  void add_line(uint32_t pc, uint32_t line) {
    line_table_[pc] = line;
  }

  struct pc_range_t {
    uint32_t pc_start_;
    uint32_t pc_end_;
    ast_decl_func_t *f_;
  };

  // pc to function mapping
  std::vector<pc_range_t> pc_range_;

  // bytecode array
  std::array<uint8_t, 1024 * 8> code_;

  // assembly streamer
  std::unique_ptr<asm_stream_t> stream_;

  // line table [PC -> Line]
  std::map<uint32_t, uint32_t> line_table_;

  // string table
  std::vector<std::string> strings_;

  // function to pc mapping
  // std::map<const ast_decl_func_t, uint32_t> funcs_;
};

} // namespace ccml
