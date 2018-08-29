#pragma once
#include <vector>

#include "ccml.h"


struct thread_t {

  thread_t(ccml_t &ccml) : ccml_(ccml), return_code_(0), finished_(true) {}

  int32_t pop() {
    const int32_t v = s_.back();
    s_.pop_back();
    return v;
  }

  void push(int32_t v) {
    s_.push_back(v);
  }

  bool prepare(const function_t &func, int32_t argc, const int32_t *argv);

  bool resume(uint32_t cycles, bool trace);

  bool finished() const {
    return finished_;
  }

  int32_t return_code() const {
    return return_code_;
  }

protected:
  friend struct vm_t;

  ccml_t &ccml_;
  int32_t return_code_;
  bool finished_;
  std::string error_;

  struct frame_t {
    int32_t sp_;
    int32_t pc_;
  };

  int32_t pc_;                    // program counter
  std::vector<int32_t> s_;        // value stack
  std::vector<frame_t> f_;        // frames
  std::vector<int32_t> globals_;  // globals

  void new_frame(int32_t pc) {
    frame_t f = {int32_t(s_.size()), pc};
    f_.push_back(f);
  }

  int32_t getv(int32_t offs) {
    const int32_t index = f_.back().sp_ + offs;
    assert(index >= 0);
    return s_[index];
  }

  void setv(int32_t offs, int32_t val) {
    const int32_t index = f_.back().sp_ + offs;
    assert(index >= 0);
    s_[index] = val;
  }

  int32_t ret(int32_t val) {
    const int32_t sval = pop();
    while (val > 0) {
      pop();
      --val;
    }
    push(sval);
    const int32_t pc = f_.back().pc_;
    f_.pop_back();
    return pc;
  }
};

struct vm_t {

  vm_t(ccml_t &c)
    : ccml_(c) {
  }

  bool execute(const function_t &func, int32_t argc, const int32_t *argv, int32_t *ret=nullptr, bool trace=false);

  // reset any stored state
  void reset();

protected:
  ccml_t &ccml_;
};
