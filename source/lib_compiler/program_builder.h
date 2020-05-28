#pragma once
#include <cstring>
#include <string>
#include <array>
#include <map>

#include "nano.h"
#include "token.h"
#include "ast.h"

#include "../lib_common/instructions.h"


namespace nano {

struct program_builder_t {

  program_builder_t(program_t &store)
    : program_(store)
    , data_(store.code_) {
  }

  void write8(const uint8_t data) {
    data_.push_back(data);
  }

  void write32(const uint32_t data) {
    data_.push_back((data >> 0) & 0xff);
    data_.push_back((data >> 8) & 0xff);
    data_.push_back((data >> 16) & 0xff);
    data_.push_back((data >> 24) & 0xff);
  }

  const uint8_t *data() const {
    return data_.data();
  }

  uint32_t head(int32_t adjust = 0) const {
    assert(adjust <= 0);
    assert(int32_t(data_.size()) + adjust >= 0);
    return int32_t(data_.size()) + adjust;
  }

  void apply_fixup(uint32_t index, int32_t value) {
    int32_t* d = (int32_t*)(data_.data() + index);
    *d = value;
  }

  int32_t add_syscall(const std::string &name) {
    auto &sys = program_.syscalls_;
    // it may already exist
    // XXX: we should change the design so that we dont insert multiple times
    for (int32_t i = 0; i < int32_t(sys.size()); ++i) {
      if (sys[i].name_ == name) {
        return i;
      }
    }
    // insert new syscall
    int32_t index = int32_t(sys.size());
    sys.push_back({name, nullptr});
    return index;
  }

  void add_global(const std::string &name, int32_t offset) {
    identifier_t ident;
    ident.name_ = name;
    ident.offset_ = offset;
    program_.globals_.push_back(ident);
  }

  void set_line(lexer_t &lexer, const token_t *line);

protected:
  program_t &program_;

  // this comes from the program_t
  std::vector<uint8_t> &data_;
};

} // namespace nano
