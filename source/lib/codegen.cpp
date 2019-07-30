#include <cstring>
#include <set>
#include <map>

#include "codegen.h"
#include "disassembler.h"
#include "lexer.h"
#include "ccml.h"
#include "errors.h"
#include "instructions.h"
#include "parser.h"


using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
namespace {
static instruction_e tok_to_ins_(token_e op) {
  switch (op) {
  case TOK_ADD: return INS_ADD;
  case TOK_SUB: return INS_SUB;
  case TOK_MUL: return INS_MUL;
  case TOK_DIV: return INS_DIV;
  case TOK_MOD: return INS_MOD;
  case TOK_AND: return INS_AND;
  case TOK_OR:  return INS_OR ;
  case TOK_NOT: return INS_NOT;
  case TOK_NEG: return INS_NEG;
  case TOK_EQ:  return INS_EQ ;
  case TOK_LT:  return INS_LT ;
  case TOK_GT:  return INS_GT ;
  case TOK_LEQ: return INS_LEQ;
  case TOK_GEQ: return INS_GEQ;
  default:
    assert(!"unknown token type");
    return instruction_e(0);
  }
}
} // namespace {}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void asm_stream_t::set_line(lexer_t &lexer, const token_t *t) {
  const uint32_t pc = pos();
  const uint32_t line = t ? t->line_no_ : lexer.stream_.line_number();
  store_.add_line(pc, line);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// essentially called during code gen when a function is encountered
//
// collect all decls and assign a stack offset to them
// also get a max frame size for the function
//
struct stack_pass_t: ast_visitor_t {

  stack_pass_t(ccml_t &ccml)
    : ccml_(ccml)
    , size_(0)
    , max_size_(0) {
  }

  void visit(ast_stmt_while_t *n) override {
    scope_.emplace_back();
    const int32_t s = size_;
    ast_visitor_t::visit(n);
    size_ = s;
    scope_.pop_back();
  }

  void visit(ast_stmt_if_t *n) override {
    scope_.emplace_back();
    const int32_t s = size_;
    ast_visitor_t::visit(n);
    size_ = s;
    scope_.pop_back();
  }

  void visit(ast_decl_func_t *n) override {
    reset();
    scope_.emplace_back();
    // add arguments
    int32_t i = -int32_t(n->args.size());
    for (auto *a : n->args) {
      auto *var = a->cast<ast_decl_var_t>();
      assert(var);
      scope_.back().insert(var);
      offsets_[var] = i++;
    }
    // visit body
    dispatch(n->body);
    scope_.pop_back();
  }

  void visit(ast_decl_var_t *n) override {
    ast_visitor_t::visit(n);
    if (!scope_.empty()) {
      scope_.back().insert(n);
      offsets_[n] = size_;
      size_ += 1;
      max_size_ = std::max(size_, max_size_);
    }
  }

  void visit(ast_program_t *p) override {
    scope_.clear();
    scope_.emplace_back();
    for (ast_node_t *n : p->children) {
      // collect var decls
      if (ast_decl_var_t *v = n->cast<ast_decl_var_t>()) {
        scope_.back().insert(v);
        // insert into global list
        offsets_[v] = globals_.size();
        globals_.insert(v);
      }
    }
  }

  void reset() {
    size_ = 0;
    max_size_ = 0;
    // remove all but globals
    for (auto itt = offsets_.begin(); itt != offsets_.end();) {
      if (itt->first->is_global()) {
        ++itt;
      }
      else {
        itt = offsets_.erase(itt);
      }
    }
  }

  // find the stack offset for a given decl
  int32_t find_offset(ast_decl_var_t *decl) {
    auto itt = offsets_.find(decl);
    if (itt == offsets_.end()) {
      assert(!"unknown decl");
    }
    return itt->second;
  }

  // given a use, find its matching decl
  ast_decl_var_t *find_decl(ast_node_t *use) const {
    if (auto *n = use->cast<ast_exp_array_t>()) {
      return n->decl;
    }
    if (auto *n = use->cast<ast_exp_ident_t>()) {
      return n->decl;
    }
    if (auto *n = use->cast<ast_stmt_assign_var_t>()) {
      return n->decl;
    }
    if (auto *n = use->cast<ast_stmt_assign_array_t>()) {
      return n->decl;
    }
    return nullptr;
  }

  int32_t get_locals_operand() const {
    return max_size_;
  }

  int32_t get_ret_operand() const {
    return max_size_ + num_args_();
  }

  const std::set<ast_decl_var_t*> &globals() const {
    return globals_;
  }

protected:
  int32_t num_args_() const {
    int32_t i = 0;
    for (const auto &a : offsets_) {
      i += a.first->is_arg();
    }
    return i;
  }

  ccml_t &ccml_;

  // decl -> offset map
  std::map<ast_decl_var_t*, int32_t> offsets_;
  // ast_decl_var_t
  std::vector<std::set<ast_decl_var_t*>> scope_;
  // global variables
  std::set<ast_decl_var_t*> globals_;
  // current stack size
  int32_t size_;
  // max stack size
  int32_t max_size_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct codegen_pass_t: ast_visitor_t {

  codegen_pass_t(ccml_t &c, asm_stream_t &stream)
    : ccml_(c)
    , stream_(stream)
    , stack_pass_(c) {}

  void visit(ast_program_t* n) override {
    stack_pass_.visit(n);
    // visit all constructs
    for (ast_node_t *c : n->children) {
      if (c->is_a<ast_decl_func_t>()) {
        dispatch(c);
      }
    }
    // process call fixups
    for (const auto &fixups : call_fixups_) {
      const int32_t offs = func_map_[fixups.first->str_];
      *fixups.second = offs;
    }
    // process globals
    for (const auto &global : stack_pass_.globals()) {
      assert(global);
      global_t g;
      g.token_ = global->name;
      g.offset_ = stack_pass_.find_offset(global);
      g.value_.v = 0;
      if (global->expr) {
        if (const auto *expr = global->expr->cast<ast_exp_lit_var_t>()) {
          g.value_.type_ = val_type_int;
          g.value_.v = expr->value;
        }
        if (const auto *expr = global->expr->cast<ast_exp_lit_str_t>()) {
          g.value_.type_ = val_type_string;
          g.value_.v = strings_.size();
          strings_.push_back(expr->value);
        }
      }
      g.size_ = global->count();
      globals_.push_back(g);
    }
  }

  void visit(ast_exp_lit_str_t* n) override {
    const uint32_t index = strings_.size();
    strings_.push_back(n->value);
    emit(INS_NEW_STR, index, n->token);
  }

  void visit(ast_exp_lit_var_t* n) override {
    emit(INS_NEW_INT, n->value, n->token);
  }

  void visit(ast_exp_none_t *n) override {
    emit(INS_NEW_NONE, n->token);
  }

  void visit(ast_exp_ident_t* n) override {
    ast_decl_var_t *decl = stack_pass_.find_decl(n);
    assert(decl);
    assert(!decl->is_array());
    const int32_t offset = stack_pass_.find_offset(decl);
    if (decl->is_local() || decl->is_arg()) {
      emit(INS_GETV, offset, n->name);
    }
    if (decl->is_global()) {
      emit(INS_GETG, offset, n->name);
    }
  }

  void visit(ast_exp_array_t* n) override {
    ast_decl_var_t *decl = stack_pass_.find_decl(n);
    assert(decl);
    const int32_t offset = stack_pass_.find_offset(decl);
    // emit the array index
    dispatch(n->index);
    // load the array object
    if (decl->is_global()) {
      emit(INS_GETG, offset, n->name);
    }
    else {
      assert(decl->is_local());  // cant have arg arrays yet
      emit(INS_GETV, offset, n->name);
    }
    // index the array
    emit(INS_GETA, n->name);
  }

  void visit(ast_exp_call_t* n) override {
    for (ast_node_t *c : n->args) {
      dispatch(c);
    }
    // find syscall
    int32_t index = 0;
    for (const auto &f : ccml_.functions()) {
      if (f.name_ == n->name->str_) {
        emit(INS_SCALL, index, n->name);
        index = -1;
        break;
      }
      ++index;
    }
    // emit regular call
    if (index != -1) {
      emit(INS_CALL, 0, n->name);
      int32_t *operand = get_fixup();
      // insert addr into map
      call_fixups_.emplace_back(n->name, operand);
    }
  }

  void visit(ast_stmt_call_t *n) override {
    dispatch(n->expr);
    // discard return value
    emit(INS_POP, 1, nullptr);
  }

  void visit(ast_exp_bin_op_t* n) override {
    dispatch(n->left);
    dispatch(n->right);
    emit(tok_to_ins_(n->op), n->token);
  }

  void visit(ast_exp_unary_op_t* n) override {
    dispatch(n->child);
    switch (n->op->type_) {
    case TOK_SUB: emit(INS_NEG, n->op); break;
    case TOK_NOT: emit(INS_NOT, n->op); break;
    default: break;
    }
  }

  void visit(ast_stmt_if_t* n) override {

    // XXX:
    //  if both blocks are empty
    //  if expr contains a call emit it
    //  if expr is const, emit specific body

    // expr
    dispatch(n->expr);

    // false jump to L0 (else)
    emit(INS_FJMP, 0, n->token); // ---> LO
    int32_t *to_L0 = get_fixup();

    // then
    dispatch(n->then_block);

    // if there is no else
    if (!n->else_block) {
      *to_L0 = pos();
      return;
    }

    // unconditional jump to end
    emit(INS_JMP, 0); // ---> L1
    int32_t *to_L1 = get_fixup();

    // else
    // L0 <---
    const int32_t L0 = pos();

    dispatch(n->else_block);

    // L1 <---
    const int32_t L1 = pos();

    // apply fixups
    *to_L0 = L0;
    *to_L1 = L1;
  }

  void visit(ast_block_t* n) override {
    for (ast_node_t *c : n->nodes) {
      dispatch(c);
    }
  }

  void visit(ast_stmt_while_t* n) override {

    // initial jump to L1 --->
    emit(INS_JMP, 0, n->token);
    int32_t *to_L1 = get_fixup();

    // L0 <---
    const int32_t L0 = pos();
    // emit the while loop body
    dispatch(n->body);

    // L1 <---
    const int32_t L1 = pos();
    // while loop condition
    dispatch(n->expr);
    // true jump to L0 --->
    emit(INS_TJMP, 0, n->token);
    int32_t *to_L0 = get_fixup();

    // apply fixups
    *to_L0 = L0;
    *to_L1 = L1;
  }

  void visit(ast_stmt_return_t *n) override {
    dispatch(n->expr);
    const int32_t space = stack_pass_.get_ret_operand();
    emit(INS_RET, space, n->token);
  }

  void visit(ast_stmt_assign_var_t *n) override {
    assert(n->expr);
    dispatch(n->expr);
    ast_decl_var_t *d = stack_pass_.find_decl(n);
    assert(d);
    const int32_t offset = stack_pass_.find_offset(d);
    if (d->is_local() || d->is_arg()) {
      emit(INS_SETV, offset, n->name);
    }
    if (d->is_global()) {
      emit(INS_SETG, offset, n->name);
    }
  }

  void visit(ast_stmt_assign_array_t *n) override {
    assert(n);
    // dispatch the value to set
    dispatch(n->expr);
    // dispatch the index
    dispatch(n->index);
    ast_decl_var_t *d = stack_pass_.find_decl(n);
    assert(d);
    const int32_t offset = stack_pass_.find_offset(d);
    if (d->is_local() || d->is_arg()) {
      emit(INS_GETV, offset, n->name);
    }
    if (d->is_global()) {
      emit(INS_GETG, offset, n->name);
    }
    emit(INS_SETA, n->name);
  }

  void visit(ast_decl_func_t* n) override {
    stack_pass_.visit(n);

    const function_t handle(n->name.c_str(), pos(), n->args.size());
    funcs_.push_back(handle);

    // insert into func map
    func_map_[n->name.c_str()] = pos();
    // parse function
    const int32_t space = stack_pass_.get_locals_operand();
    if (space > 0) {
      emit(INS_LOCALS, space, n->token);
    }
    bool is_return = false;
    dispatch(n->body);
    if (n->body) {
      for (ast_node_t *c : n->body->nodes) {
        is_return = c->is_a<ast_stmt_return_t>();
      }
    }
    if (!is_return) {
      emit(INS_NEW_INT, 0, nullptr);
      emit(INS_RET, stack_pass_.get_ret_operand(), nullptr);
    }
  }

  void visit(ast_decl_var_t* n) override {
    const int32_t offset = stack_pass_.find_offset(n);
    if (n->is_array()) {
      emit(INS_NEW_ARY, n->size->val_, n->name);
      emit(INS_SETV, offset, n->name);
    }
    if (n->expr) {
      dispatch(n->expr);
      emit(INS_SETV, offset, n->name);
    }
  }

  const std::vector<function_t> &functions() const {
    return funcs_;
  }

  const std::vector<global_t> &globals() const {
    return globals_;
  }

  const std::vector<std::string> &strings() const {
    return strings_;
  }

protected:

  // emit into the code stream
  void emit(instruction_e ins, const token_t *t = nullptr);
  void emit(instruction_e ins, int32_t v, const token_t *t = nullptr);

  // return the current output head
  int32_t pos() const {
    return stream_.pos();
  }

  // return a reference to the last instructions operand
  int32_t *get_fixup() {
    return reinterpret_cast<int32_t *>(stream_.head(-4));
  }

  std::vector<global_t> globals_;
  std::vector<function_t> funcs_;
  std::vector<std::string> strings_;
  std::map<std::string, int32_t> func_map_;
  std::vector<std::pair<const token_t *, int32_t *>> call_fixups_;
  stack_pass_t stack_pass_;
  asm_stream_t &stream_;
  ccml_t &ccml_;
};

void codegen_pass_t::emit(instruction_e ins, const token_t *t) {
  stream_.set_line(ccml_.lexer(), t);
  // encode this instruction
  switch (ins) {
  case INS_ADD:
  case INS_SUB:
  case INS_MUL:
  case INS_DIV:
  case INS_MOD:
  case INS_AND:
  case INS_OR:
  case INS_NOT:
  case INS_LT:
  case INS_GT:
  case INS_LEQ:
  case INS_GEQ:
  case INS_EQ:
  case INS_NEG:
  case INS_SETA:
  case INS_GETA:
  case INS_NEW_NONE:
    stream_.write8(uint32_t(ins));
    break;
  default:
    assert(!"unknown instruction");
  }
}

void codegen_pass_t::emit(instruction_e ins, int32_t v, const token_t *t) {
  stream_.set_line(ccml_.lexer(), t);
  // encode this instruction
  switch (ins) {
  case INS_JMP:
  case INS_FJMP:
  case INS_TJMP:
  case INS_CALL:
  case INS_RET:
  case INS_SCALL:
  case INS_POP:
  case INS_NEW_ARY:
  case INS_NEW_INT:
  case INS_NEW_STR:
  case INS_LOCALS:
  case INS_ACCV:
  case INS_GETV:
  case INS_SETV:
  case INS_GETG:
  case INS_SETG:
    stream_.write8(uint8_t(ins));
    stream_.write32(v);
    break;
  default:
    assert(!"unknown instruction");
  }
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
codegen_t::codegen_t(ccml_t &c, asm_stream_t &s)
  : ccml_(c)
  , stream_(s)
{
}

bool codegen_t::run(ast_program_t &program, error_t &error) {
  codegen_pass_t cg{ccml_, stream_};
  try {
    cg.visit(&program);
  }
  catch (const error_t &e) {
    error = e;
    return false;
  }
  // add functions to ccml
  for (const auto &f : cg.functions()) {
    ccml_.add_(f);
  }
  // add globals to ccml
  for (const auto &g : cg.globals()) {
    ccml_.add_(g);
  }
  // add strings to ccml
  for (const auto &s : cg.strings()) {
    ccml_.add_(s);
  }

  return true;
}

void codegen_t::reset() {
}
