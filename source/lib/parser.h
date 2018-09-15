#pragma once
#include <vector>
#include <array>

#include "ccml.h"
#include "token.h"
#include "assembler.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct scope_list_t {

  scope_list_t(ccml_t &ccml)
    : max_depth_(0)
    , ccml_(ccml)
  {
    reset();
  }

  void enter() {
    if (var_head_.empty()) {
      var_head_.emplace_back(0);
    } else {
      const int32_t back = head();
      var_head_.emplace_back(back);
    }
  }

  void leave() {
    assert(!var_head_.empty());
    // current code position
    const uint32_t pos = ccml_.assembler().pos();
    // end of this scope
    const int32_t end = head();
    // step back one scope
    var_head_.pop_back();
    // find start of this stack frame
    const int32_t start = var_head_.empty() ? 0 : head();
    // for all variables that are discarded, add their debug info
    for (int32_t i = start; i < end; ++i) {
      // code location where we exit
      vars_[i].end = pos;
      ccml_.assembler().add_ident(vars_[i]);
    }
  }

  // add a variable to this scope
  void var_add(const token_t &tok, uint32_t count) {
    assert(head() < int32_t(vars_.size()));
    // check if we are in global scope or not
    const bool is_global = var_head_.size() <= 1;
    // return global offset or frame offset
    const int32_t offset = is_global ? global_count() : var_count();
    // push back this identifier
    auto ident = identifier_t(&tok, offset, is_global, count);
    ident.start = ccml_.assembler().pos();
    vars_[head()++] = ident;
    // update max depth
    max_depth_ = std::max(max_depth_, var_count());
  }

  // return the current size of the globals
  int32_t global_count() const {
    int32_t count = 0;
    if (var_head_.size() > 0) {
      for (int32_t i = 0; i < var_head_[0]; ++i) {
        count += vars_[i].count;
      }
    }
    return count;
  }

  // return space used for variables in stack frame
  int32_t var_count() const {
    assert(var_head_.size() > 0);
    int32_t count = 0;
    // note: start from scope level 1 so that we dont collect global variables
    for (int i = var_head_[0]; i < head(); ++i) {
      count += vars_[i].count;
    }
    return count;
  }

  // add an argument to this function
  void arg_add(const token_t *tok) {
    identifier_t ident(tok, 0, false, 1);
    ident.start = ccml_.assembler().pos();
    args_[arg_head_++] = ident;
  }

  // get the current number of arguments
  int32_t arg_count() const {
    return arg_head_;
  }

  // calculate argument offsets from frame pointer
  // note: dont push any more arguments
  void arg_calc_offsets() {
    for (int32_t i = 0; i < arg_head_; ++i) {
      identifier_t &arg = args_[i];
      arg.offset = i - arg_head_;
    }
  }

  const identifier_t *find_ident(const token_t &name) const {
    // note: back to front search could enable shadowing
    // search for locals
    for (int32_t i = head() - 1; i >= 0; --i) {
      const auto &ident = vars_[i];
      if (ident.token->str_ == name.str_) {
        return &vars_[i];
      }
    }
    // search for arguments
    for (int32_t i = 0; i < arg_head_; ++i) {
      if (args_[i].token->str_ == name.str_) {
        return &args_[i];
      }
    }
    // none found
    return nullptr;
  }

  void reset() {
    // reset vars
    max_depth_ = 0;
    var_head_.clear();
    // reset args
    arg_head_ = 0;
  }

  void leave_function() {
    const int32_t pos = ccml_.assembler().pos();
    for (int32_t i = 0; i < arg_head_; ++i) {
      args_[i].end = pos;
      ccml_.assembler().add_ident(args_[i]);
    }
    arg_head_ = 0;
    max_depth_ = 0;
  }

  // return the max stack depth size encountered
  int32_t max_depth() const {
    return max_depth_;
  }

protected:
  const int32_t & head() const {
    return var_head_.back();
  }

  int32_t & head() {
    assert(!var_head_.empty());
    return var_head_.back();
  }

  ccml_t &ccml_;

  std::array<identifier_t, 64> vars_;
  std::vector<int32_t> var_head_;
  int32_t max_depth_;

  std::array<identifier_t, 64> args_;
  int32_t arg_head_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct function_t {
  ccml_syscall_t sys_;
  std::string name_;
  int32_t pos_;
  uint32_t num_args_;
  uint32_t id_;
  std::vector<int32_t*> ret_fixup_;

  function_t(const std::string &name, ccml_syscall_t sys, int32_t num_args,
             int32_t id)
    : sys_(sys), name_(name), pos_(-1), num_args_(num_args), id_(id) {}

  function_t(const std::string &name, int32_t pos, int32_t num_args, int32_t id)
    : sys_(nullptr), name_(name), pos_(pos), num_args_(num_args), id_(id) {}

  void add_return_fixup(int32_t *r) {
    ret_fixup_.push_back(r);
  }

  void fix_return_operands(const int32_t count) {
    for (int32_t *r : ret_fixup_) {
      *r = count;
    }
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct global_t {
  const token_t *token_;
  int32_t offset_;
  int32_t value_;
  int32_t size_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct parser_t {

  parser_t(ccml_t &c);

  // parse all tokens stored in the lexer
  bool parse(struct error_t &error);

  // reset any stored state
  void reset();

  // add a syscall function
  void add_function(const std::string &name, ccml_syscall_t func, uint32_t num_args);

  // find a function by name
  // if `can_fail == false` it will report an error
  const function_t *find_function(const token_t *name, bool can_fail = false) const;
  const function_t *find_function(const std::string &name) const;
  const function_t *find_function(uint32_t id) const;

  // return a list of functions
  const std::vector<function_t> &functions() const {
    return funcs_;
  }

  // return the number of globals
  int32_t global_size() const {
    int32_t count = 0;
    for (const auto &g : globals()) {
      count += g.size_;
    }
    return count;
  }

  // return a list of globals
  const std::vector<global_t> globals() const {
    return global_;
  }

protected:
  ccml_t &ccml_;

  // list of parsed function
  std::vector<function_t> funcs_;

  // operator stack for expression parsing
  std::vector<const token_t*> op_stack_;
  // expression stack
  std::vector<ast_node_t*> exp_stack_;

    // list of parsed globals
  std::vector<global_t> global_;
  // scope manager
  scope_list_t scope_;

  // parse specific language constructs
  ast_node_t* parse_array_get_(const token_t &name);
  ast_node_t* parse_array_set_(const token_t &name);
  ast_node_t* parse_function_(const token_t &t);
  ast_node_t* parse_stmt_();
  ast_node_t* parse_return_(const token_t &t);
  ast_node_t* parse_while_(const token_t &t);
  ast_node_t* parse_if_(const token_t &t);
  ast_node_t* parse_call_(const token_t &name);
  ast_node_t* parse_assign_(const token_t &name);
  ast_node_t* parse_decl_array_(const token_t &name);
  ast_node_t* parse_decl_(const token_t &t);
  ast_node_t* parse_expr_();
  void parse_expr_ex_(uint32_t tide);
  void parse_lhs_();
  ast_node_t* parse_global_();

  // return true if next token is an operator
  bool is_operator_() const;

  // get the precedence of this operator
  int32_t op_type_(token_e type) const;

  // XXX: move into subclass
  // push an operator on the stack
  void op_push_(const token_t* op, uint32_t tide);
  // pop all operators off the stack
  void op_pop_all_(uint32_t tide);
  // reduce an operation
  void op_reduce_();
};

} // namespace ccml
