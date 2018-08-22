#pragma once
#include "ccml.h"
#include "token.h"

struct parser_t {

  struct function_t {
    ccml_syscall_t sys_;
    std::string name_;
    int32_t pos_;
    int32_t frame_;
    std::vector<std::string> ident_;

    function_t() : sys_(nullptr), name_(), pos_(), frame_() {}

    int32_t frame_size() const { return ident_.size(); }

    int32_t find(const std::string &str) {
      for (uint32_t i = 0; i < ident_.size(); i++) {
        if (str == ident_[i]) {
          return i - frame_;
        }
      }
      throws("unknown identifier");
      return -1;
    }
  };

  parser_t(ccml_t &c) : ccml_(c) {}

  bool parse();

  void add_function(const std::string &name, ccml_syscall_t func);
  const function_t *find_function(const std::string &name);

protected:
  ccml_t &ccml_;
  //    uint32_t op_head_;
  std::vector<function_t> funcs_;
  std::vector<token_e> op_stack_;

  void parse_function();
  void parse_stmt();
  void parse_while();
  void parse_if();
  void parse_call(token_t *name);
  void parse_assign(token_t *name);
  void parse_decl();
  void parse_expr();
  void parse_expr_ex(uint32_t tide);
  void parse_lhs();

  bool is_operator();
  int32_t op_type(token_e type);

  void op_push(token_e op, uint32_t tide);
  void op_popall(uint32_t tide);

  function_t &func();
};
