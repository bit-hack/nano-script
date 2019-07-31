#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <map>

#include "value.h"
#include "instructions.h"


namespace ccml {

enum instruction_e;
struct instruction_t;

struct error_manager_t;
struct error_t;

struct lexer_t;
struct token_t;
struct token_stream_t;

struct parser_t;
struct function_t;
struct identifier_t;

struct ast_t;
struct ast_block_t;
struct ast_node_t;
struct ast_program_t;
struct ast_exp_ident_t;
struct ast_exp_array_t;
struct ast_exp_call_t;
struct ast_exp_bin_op_t;
struct ast_exp_unary_op_t;
struct ast_stmt_if_t;
struct ast_stmt_while_t;
struct ast_stmt_return_t;
struct ast_stmt_assign_var_t;
struct ast_stmt_assign_array_t;
struct ast_decl_func_t;
struct ast_decl_var_t;

struct asm_stream_t;
struct codegen_t;

struct disassembler_t;

struct thread_t;
struct vm_t;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct global_t {
  const token_t *token_;
  int32_t offset_;
  value_t value_;
  int32_t size_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct identifier_t {

  identifier_t()
    : offset(0)
    , count(0)
    , token(nullptr)
    , is_global(false) {}

  identifier_t(const token_t *t, int32_t o, bool is_glob, int32_t c = 1)
    : offset(o)
    , count(c)
    , token(t)
    , is_global(is_glob) {}

  bool is_array() const {
    return count > 1;
  }

  // start and end valiity range
  uint32_t start, end;

  // offset from frame pointer
  int32_t offset;
  // number of items (> 1 == array)
  int32_t count;
  // identifier token
  const token_t *token;
  // if this is a global variable
  bool is_global;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
typedef void(*ccml_syscall_t)(struct thread_t &thread);

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct function_t {
  std::string name_;
  int32_t pos_;
  ccml_syscall_t sys_;
  uint32_t num_args_;

  bool is_syscall() const {
    return sys_ != nullptr;
  }

  function_t(const std::string &name, ccml_syscall_t sys, int32_t num_args)
    : name_(name), pos_(-1), sys_(sys), num_args_(num_args) {}

  function_t(const std::string &name, int32_t pos, int32_t num_args)
    : name_(name), pos_(pos), sys_(nullptr), num_args_(num_args) {}
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct code_store_t {

  code_store_t();

  const uint8_t *data() const {
    return code_.data();
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

  bool active_vars(const uint32_t pc,
                   std::vector<const identifier_t *> &out) const;

protected:
  friend struct asm_stream_t;

  uint8_t *data() {
    return code_.data();
  }

  void add_line(uint32_t pc, uint32_t line) {
    line_table_[pc] = line;
  }

  std::array<uint8_t, 1024 * 8> code_;
  std::unique_ptr<asm_stream_t> stream_;

  // line table [PC -> Line]
  std::map<uint32_t, uint32_t> line_table_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct ccml_t {

  ccml_t();
  ~ccml_t();

  // accessors
  error_manager_t &errors()       { return *errors_; }
  code_store_t    &store()        { return store_; }
  lexer_t         &lexer()        { return *lexer_; }
  parser_t        &parser()       { return *parser_; }
  ast_t           &ast()          { return *ast_; }
  codegen_t       &codegen()      { return *codegen_; }
  disassembler_t  &disassembler() { return *disassembler_; }
  vm_t            &vm()           { return *vm_; }

  bool build(const char *source, error_t &error);

  void reset();

  const uint8_t *code() const {
    return store_.data();
  }

  const function_t *find_function(const std::string &name) const;

  void add_function(const std::string &name, ccml_syscall_t sys, int32_t num_args);

  const std::vector<function_t> &functions() const {
    return functions_;
  }

//  const std::vector<global_t> &globals() const {
//    return globals_;
//  }

  const std::vector<std::string> &strings() const {
    return strings_;
  }

private:
  friend struct vm_t;
  friend struct lexer_t;
  friend struct parser_t;
  friend struct codegen_t;
  friend struct disassembler_t;
  friend struct token_stream_t;
  friend struct error_manager_t;

  void add_builtins_();

  void add_(const function_t &func) {
    functions_.push_back(func);
  }

//  void add_(const global_t &ident) {
//    globals_.push_back(ident);
//  }

  void add_(const std::string &string) {
    strings_.push_back(string);
  }

  // the code store
  code_store_t store_;
  std::vector<function_t> functions_;
//  std::vector<global_t> globals_;
  std::vector<std::string> strings_;

  std::unique_ptr<error_manager_t> errors_;
  std::unique_ptr<lexer_t>         lexer_;
  std::unique_ptr<parser_t>        parser_;
  std::unique_ptr<ast_t>           ast_;
  std::unique_ptr<codegen_t>       codegen_;
  std::unique_ptr<disassembler_t>  disassembler_;
  std::unique_ptr<vm_t>            vm_;
};

} // namespace ccml
