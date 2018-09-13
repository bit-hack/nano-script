#include <cassert>

#include "errors.h"
#include "assembler.h"


using namespace ccml;


// TODO:
//    GETV x, SETV x
//    x = x ... ?
//    INS_LOCALS 0

namespace {
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
bool is_binary_op(const instruction_e ins) {
  switch (ins) {
  case INS_ADD:
  case INS_SUB:
  case INS_MUL:
  case INS_DIV:
  case INS_MOD:
  case INS_AND:
  case INS_OR:
  case INS_EQ:
  case INS_LT:
  case INS_GT:
  case INS_LEQ:
  case INS_GEQ:
    return true;
  default:
    return false;
  }
}

bool is_unary_op(const instruction_e ins) {
  switch (ins) {
  case INS_NOT:
  case INS_NEG:
    return true;
  default:
    return false;
  }
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct optimization_t {

  optimization_t(ccml_t &ccml, std::vector<ccml::instruction_t> &ph)
    : ccml_(ccml)
    , ph_(ph)
  {
  }

  virtual bool run() = 0;

protected:
  void remove_(size_t start, size_t count) {
    assert((start + count) <= ph_.size());
    ph_.erase(ph_.begin() + start, ph_.begin() + start + count);
  }

  void insert_(size_t index, const instruction_t &ins) {
    ph_.insert(ph_.begin() + index, ins);
  }

  ccml_t &ccml_;
  std::vector<ccml::instruction_t> &ph_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct opt_not_not_t : public optimization_t {

  opt_not_not_t(ccml_t &ccml, std::vector<ccml::instruction_t> &ph)
      : optimization_t(ccml, ph) {}

  bool run() override {
    for (int32_t i = 0; i < int32_t(ph_.size()) - 1; ++i) {
      // if this matches
      if (ph_[i + 0].opcode == INS_NOT && ph_[i + 1].opcode == INS_NOT) {
        // remove the not not
        remove_(i, 2);
        return true;
      }
    }
    return false;
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct opt_unary_op_t: public optimization_t {

  opt_unary_op_t(ccml_t &ccml, std::vector<ccml::instruction_t> &ph)
    : optimization_t(ccml, ph)
  {
  }

  bool can_match(int32_t i) const {
    if (ph_[i + 0].opcode != ccml::INS_CONST)
      return false;
    return is_unary_op(ph_[i + 1].opcode);
  }

  int32_t result(instruction_e ins, int32_t a, const token_t &tok) const {
    switch (ins) {
    case INS_NOT: return !a;
    case INS_NEG: return -a;
    default:
      assert(!"unexpected unary operation");
      return 0;
    }
  }

  bool run() override {
    for (int32_t i = 0; i < int32_t(ph_.size()) - 1; ++i) {
      // if this matches
      if (!can_match(i)) {
        continue;
      }
      // get operands
      const auto op_a = ph_[i + 0];
      const auto inst = ph_[i + 1];
      remove_(i, 2);
      // insert constant
      const token_t *token = op_a.token;
      const int32_t value = result(inst.opcode, op_a.operand, *token);
      insert_(i, instruction_t{INS_CONST, value, token});
      return true;
    }
    return false;
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct opt_binary_op_t: public optimization_t {

  opt_binary_op_t(ccml_t &ccml, std::vector<ccml::instruction_t> &ph)
    : optimization_t(ccml, ph)
  {
  }

  bool can_match(const int32_t i) const {
    if (ph_[i + 0].opcode != ccml::INS_CONST)
      return false;
    if (ph_[i + 1].opcode != ccml::INS_CONST)
      return false;
    return is_binary_op(ph_[i + 2].opcode);
  }

  int32_t result(instruction_e ins, int32_t a, int32_t b, const token_t &tok) const {
    switch (ins) {
    case INS_ADD: return a + b;
    case INS_SUB: return a - b;
    case INS_MUL: return a * b;
    case INS_AND: return a && b;
    case INS_OR:  return a || b;
    case INS_EQ:  return a == b;
    case INS_LT:  return a < b;
    case INS_GT:  return a > b;
    case INS_LEQ: return a <= b;
    case INS_GEQ: return a >= b;
    case INS_DIV:
      if (b == 0) {
        // divide by zero
        ccml_.errors().constant_divie_by_zero(tok);
      }
      return a / b;
    case INS_MOD:
      if (b == 0) {
        // divide by zero
        ccml_.errors().constant_divie_by_zero(tok);
      }
      return a % b;
    default:
      assert(!"unexpected binary operation");
      return 0;
    }
  }

  bool run() override {
    for (int32_t i = 0; i < int32_t(ph_.size()) - 2; ++i) {
      // if this matches
      if (!can_match(i)) {
        continue;
      }
      // get instructions
      const auto op_a = ph_[i + 0];
      const auto op_b = ph_[i + 1];
      const auto inst = ph_[i + 2];
      remove_(i, 3);
      // insert constant
      const token_t *token = op_a.token;
      const int32_t value = result(inst.opcode, op_a.operand, op_b.operand, *token);
      insert_(i, instruction_t{INS_CONST, value, token});
      return true;
    }
    return false;
  }
};

} // namespace

namespace ccml {

size_t size_start, size_end;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void assembler_t::peep_hole_optimize_() {
  // refer to the peephole
  std::vector<instruction_t> &ph = peep_hole_;

  size_start += ph.size();

  // run optimizations
  while (opt_not_not_t  {ccml_, ph}.run());
  while (opt_unary_op_t {ccml_, ph}.run());
  while (opt_binary_op_t{ccml_, ph}.run());

  size_end += ph.size();
}

} // namespace ccml
