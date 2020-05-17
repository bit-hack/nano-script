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

namespace ccml {

struct codegen_pass_t: ast_visitor_t {

  codegen_pass_t(ccml_t &c, program_builder_t &stream)
    : ccml_(c)
    , stream_(stream)
    , funcs_(c.program_.functions())
    , strings_(c.program_.strings()) {}

  void set_decl_(ast_decl_var_t *decl, const token_t *t = nullptr) {
    assert(decl);
    // const shouldnt even make it into the backend let alone ever be set
    assert(!decl->is_const);
    if (decl->is_local() || decl->is_arg()) {
      emit(INS_SETV, decl->offset, t);
    }
    if (decl->is_global()) {
      emit(INS_SETG, decl->offset, t);
    }
  }

  void get_decl_(ast_decl_var_t *decl, const token_t *t = nullptr) {
    assert(decl);
    // const shouldnt even make it into the backend
    assert(!decl->is_const);
    if (decl->is_local() || decl->is_arg()) {
      emit(INS_GETV, decl->offset, t);
    }
    if (decl->is_global()) {
      emit(INS_GETG, decl->offset, t);
    }
  }

  void get_func_(ast_decl_func_t *func, const token_t *t = nullptr) {
    assert(func);
    if (func->syscall) {
      const auto &syscalls = ccml_.program_.syscall();
      for (size_t i = 0; i < syscalls.size(); ++i) {
        if (syscalls[i] == func->syscall) {
          emit(INS_NEW_SCALL, i, t);
          return;
        }
      }
      assert(!"syscall not in table");
    }
    else {
      emit(INS_NEW_FUNC, 0, t);
      uint32_t operand = get_fixup();
      // insert addr into map
      call_fixups_.emplace_back(func->token, operand);
    }
  }

  void visit(ast_program_t* n) override {
    // visit all constructs
    for (ast_node_t *c : n->children) {
      if (c->is_a<ast_decl_func_t>()) {
        dispatch(c);
      }
      if (c->is_a<ast_decl_var_t>()) {
        dispatch(c);
      }
    }
    // process call fixups
    for (const auto &fixups : call_fixups_) {
      const int32_t offs = func_map_[fixups.first->str_];
      stream_.apply_fixup(fixups.second, offs);
    }
  }

  void visit(ast_exp_lit_str_t* n) override {
    const uint32_t index = strings_.size();
    strings_.push_back(n->value);
    emit(INS_NEW_STR, index, n->token);
  }

  void visit(ast_exp_lit_float_t *n) override {
    const uint32_t out = *(const uint32_t*)(&n->val);
    emit(INS_NEW_FLT, out, n->token);
  }

  void visit(ast_exp_lit_var_t* n) override {
    emit(INS_NEW_INT, n->val, n->token);
  }

  void visit(ast_exp_none_t *n) override {
    emit(INS_NEW_NONE, n->token);
  }

  void visit(ast_exp_ident_t* n) override {
    if (ast_decl_var_t *decl = n->decl->cast<ast_decl_var_t>()) {
      assert(!decl->is_array());
      assert(!decl->is_const);
      get_decl_(decl, n->name);
      return;
    }
    if (ast_decl_func_t *func = n->decl->cast<ast_decl_func_t>()) {
      get_func_(func, n->name);
      return;
    }
    assert(!"unknown decl type");
  }

  void visit(ast_exp_array_t* n) override {
    ast_decl_var_t *decl = n->decl;
    assert(decl);
    assert(!decl->is_const);
    // emit the array index
    dispatch(n->index);
    // load the array object
    get_decl_(decl, n->name);
    // index the array
    emit(INS_GETA, n->name);
  }

  void visit(ast_exp_call_t* n) override {
    assert(n && n->callee);

    const size_t num_args = n->args.size();
    // visit all of the arguments
    for (ast_node_t *c : n->args) {
      dispatch(c);
    }

    // emit direct calls when we can
    if (ast_exp_ident_t *ident = n->callee->cast<ast_exp_ident_t>()) {
      if (ast_decl_func_t *func = ident->decl->cast<ast_decl_func_t>()) {
        // this should have been checked beforehand
        assert(func->args.size() == num_args);
        if (func->syscall) {
          // emit a syscall
          uint32_t index = stream_.add_syscall(func->syscall);
          emit(INS_SCALL, int32_t(num_args), index, ident->name);
        }
        else {
          // emit regular call
          emit(INS_CALL, int32_t(num_args), 0, n->token);
          uint32_t operand = get_fixup();
          // insert addr into map
          call_fixups_.emplace_back(func->token, operand);
        }
        return;
      }
    }

    // emit an indirect call in the worst case
    dispatch(n->callee);
    emit(INS_ICALL, int32_t(num_args), n->token);
  }

  void visit(ast_stmt_call_t *n) override {
    dispatch(n->expr);
    // discard return value
    emit(INS_POP, 1, n->expr->token);
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

    // format:
    //
    // if (!<expr>) ----.
    // <expr>           |
    // L0 <-------------'
    //
    //
    // if (!<expr>) ----.
    // <expr>           |
    // jmp -----------. |
    // L0 <-------------'
    // <expr>         |
    // L1 <-----------'

    // expr
    dispatch(n->expr);

    // false jump to L0 (else)
    emit(INS_FJMP, 0, n->token); // ---> LO
    uint32_t to_L0 = get_fixup();

    // then
    dispatch(n->then_block);

    // if there is no else
    if (!n->else_block) {
      stream_.apply_fixup(to_L0, pos());
      return;
    }

    // unconditional jump to end
    emit(INS_JMP, 0); // ---> L1
    uint32_t to_L1 = get_fixup();

    // else
    // L0 <---
    const int32_t L0 = pos();

    dispatch(n->else_block);

    // L1 <---
    const int32_t L1 = pos();

    // apply fixups
    stream_.apply_fixup(to_L0, L0);
    stream_.apply_fixup(to_L1, L1);
  }

  void visit(ast_block_t* n) override {
    for (ast_node_t *c : n->nodes) {
      dispatch(c);
    }
  }

  void visit(ast_stmt_while_t* n) override {

    // format:
    //
    // jmp -----.
    // L0 <-----|-.
    // <body>   | |
    // L1 <-----' |
    // if <exp> --'

    // initial jump to L1 --->
    emit(INS_JMP, 0, n->token);
    uint32_t to_L1 = get_fixup();

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
    uint32_t to_L0 = get_fixup();

    // apply fixups
    stream_.apply_fixup(to_L0, L0);
    stream_.apply_fixup(to_L1, L1);
  }

  void visit(ast_stmt_for_t* n) override {

    // format:
    // 
    // <name> = <start>
    // jmp ----------------.
    // L0 <----------------|----.
    // <body>              |    |
    // L1 <----------------'    |
    // <name> += 1              |
    // if (<name> < <end>) jmp -'
    //

    assert(n->decl);

    dispatch(n->start);
    set_decl_(n->decl, n->name);

    // initial jump to L1 --->
    emit(INS_JMP, 0, n->token);
    uint32_t to_L1 = get_fixup();

    // L0 <---
    const int32_t L0 = pos();
    // emit the for loop body
    dispatch(n->body);

    // L1 <---

    // increment n->decl
    get_decl_(n->decl, n->token);
    emit(INS_NEW_INT, 1, n->token);
    emit(INS_ADD, n->token);
    set_decl_(n->decl, n->token);

    // check end condition
    const int32_t L1 = pos();
    get_decl_(n->decl, n->token);
    dispatch(n->end);
    emit(INS_LT, n->token);

    // true jump to L0 --->
    emit(INS_TJMP, 0, n->token);
    uint32_t to_L0 = get_fixup();

    // apply fixups
    stream_.apply_fixup(to_L0, L0);
    stream_.apply_fixup(to_L1, L1);
  }

  void visit(ast_stmt_return_t *n) override {
    if (n->expr) {
      dispatch(n->expr);
    } else {
      emit(INS_NEW_NONE, n->token);
    }
    const auto *func = stack.front()->cast<ast_decl_func_t>();
    assert(func);
    const int32_t space = func->args.size() + func->stack_size;
    emit(INS_RET, space, n->token);
  }

  void visit(ast_stmt_assign_var_t *n) override {
    assert(n->expr);
    dispatch(n->expr);
    ast_decl_var_t *d = n->decl;
    assert(d);
    assert(!d->is_const);
    set_decl_(d, n->name);
  }

  void visit(ast_stmt_assign_array_t *n) override {
    assert(n);
    // dispatch the value to set
    dispatch(n->expr);
    // dispatch the index
    dispatch(n->index);
    ast_decl_var_t *d = n->decl;
    assert(d);
    assert(!d->is_const);
    get_decl_(d, n->name);
    emit(INS_SETA, n->name);
  }

  void visit(ast_decl_func_t* n) override {
    // syscalls dont get emitted
    if (n->syscall) {
      return;
    }

    function_t *func = ccml_.program_.function_find(n->name);
    assert(func);
    func->code_start_ = pos();
    func->code_end_ = pos();

    // insert into func map
    func_map_[n->name.c_str()] = pos();

    // init is handled separately as a special case
    if (n->name == "@init") {
      visit_init(n, func);
      return;
    }

    // parse function
    if (n->stack_size > 0) {
      emit(INS_LOCALS, n->stack_size, n->token);
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
      const auto *f = stack.front()->cast<ast_decl_func_t>();
      const int32_t operand = f->args.size() + f->stack_size;
      emit(INS_RET, operand, nullptr);
    }

    func->code_end_ = pos();
  }

  void visit(ast_decl_var_t* n) override {
    // collect the globals
    if (n->is_global()) {
      stream_.add_global(n->name->str_, n->offset);
    }
    // must be local
    else {
      if (n->is_const) {
        // nothing to do for const variables
        return;
      }
      if (n->is_array()) {
        emit(INS_NEW_ARY, n->count(), n->name);
        set_decl_(n, n->name);
      } else {
        assert(n->is_local());
        if (n->expr) {
          dispatch(n->expr);
          emit(INS_SETV, n->offset, n->name);
        }
      }
    }
  }

  const std::vector<function_t> &functions() const {
    return funcs_;
  }

  const std::vector<std::string> &strings() const {
    return strings_;
  }

protected:

  // emit into the code stream
  void emit(instruction_e ins, const token_t *t = nullptr);
  void emit(instruction_e ins, int32_t o1, const token_t *t = nullptr);
  void emit(instruction_e ins, int32_t o1, int32_t o2, const token_t *t = nullptr);

  // return the current output head
  int32_t pos() const {
    return stream_.head();
  }

  // return a index of the last instructions operand
  uint32_t get_fixup() const {
    return stream_.head(-4);
  }

  // handle @init function as a special case
  void visit_init(ast_decl_func_t* a, function_t *func) {

    int num_globals = 0;
    for (ast_node_t *n : ccml_.ast().program.children) {
      if (auto *decl = n->cast<ast_decl_var_t>()) {
        if (!decl->is_const) {
          num_globals += 1;
        }
      }
    }

    emit(INS_GLOBALS, num_globals, nullptr);

    int32_t offset = 0;
    for (ast_node_t *n : ccml_.ast().program.children) {
      if (auto *d = n->cast<ast_decl_var_t>()) {
        if (d->is_const) {
          continue;
        }
        if (d->is_array()) {
          emit(INS_NEW_ARY, d->count(), d->name);
          emit(INS_SETG, offset, d->name);
        }
        ++offset;
      }
    }

    ast_visitor_t::visit(a);

    // return
    emit(INS_NEW_INT, 0, nullptr);
    const auto *ast_func = stack.front()->cast<ast_decl_func_t>();
    const int32_t operand = ast_func->args.size() + ast_func->stack_size;
    emit(INS_RET, operand, nullptr);

    func->code_end_ = pos();
  }

  ccml_t &ccml_;
  program_builder_t &stream_;
  std::vector<function_t> &funcs_;
  std::vector<std::string> &strings_;
  std::map<std::string, int32_t> func_map_;
  std::vector<std::pair<const token_t *, uint32_t>> call_fixups_;
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
    stream_.write8(uint8_t(ins));
    break;
  default:
    assert(!"unknown instruction");
  }
}

void codegen_pass_t::emit(instruction_e ins, int32_t o1, const token_t *t) {
  stream_.set_line(ccml_.lexer(), t);
  // encode this instruction
  switch (ins) {
  case INS_JMP:
  case INS_FJMP:
  case INS_TJMP:
  case INS_RET:
  case INS_POP:
  case INS_NEW_ARY:
  case INS_NEW_INT:
  case INS_NEW_STR:
  case INS_NEW_FLT:
  case INS_NEW_FUNC:
  case INS_NEW_SCALL:
  case INS_LOCALS:
  case INS_GLOBALS:
  case INS_GETV:
  case INS_SETV:
  case INS_GETG:
  case INS_SETG:
  case INS_ICALL:
    stream_.write8(uint8_t(ins));
    stream_.write32(o1);
    break;
  default:
    assert(!"unknown instruction");
  }
}

void codegen_pass_t::emit(instruction_e ins, int32_t o1, int32_t o2, const token_t *t) {
  stream_.set_line(ccml_.lexer(), t);
  // encode this instruction
  switch (ins) {
  case INS_SCALL:
  case INS_CALL:
    stream_.write8(uint8_t(ins));
    stream_.write32(o1);  // num args
    stream_.write32(o2);  // target
    break;
  default:
    assert(!"unknown instruction");
  }
}

} // namespace ccml

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
codegen_t::codegen_t(ccml_t &c, program_t &prog)
  : ccml_(c)
  , stream_(prog)
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
  return true;
}
