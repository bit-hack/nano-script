#pragma once
#include <cstring>
#include <string>
#include <array>
#include <map>

#include "ccml.h"
#include "token.h"
#include "instructions.h"
#include "ast.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// the asm stream bridges the assembler and the code store
struct asm_stream_t {

  asm_stream_t(code_store_t &store)
    : store_(store)
    , end(store.end())
    , start(store.data())
    , ptr(store.data())
  {
  }

  bool write8(const uint8_t data) {
    if (end - ptr > 1) {
      *ptr = data;
      ptr += 1;
      return true;
    }
    return false;
  }

  bool write32(const uint32_t data) {
    if (end - ptr > 4) {
      memcpy(ptr, &data, 4);
      ptr += 4;
      return true;
    }
    return false;
  }

  const uint8_t *tail() const {
    return end;
  }

  size_t written() const {
    return ptr - start;
  }

  const uint8_t *data() const {
    return start;
  }

  size_t size() const {
    return end - start;
  }

  uint32_t pos() const {
    return ptr - start;
  }

  uint8_t *head(int32_t adjust) const {
    uint8_t *p = ptr + adjust;
    assert(p < end && p >= start);
    return p;
  }

  // set the line number for the current pc
  // if line is nullptr then current lexer line is used
  void set_line(lexer_t &lexer, const token_t *line);

  // add identifier
  void add_ident(const identifier_t &ident);

protected:
  code_store_t &store_;
  std::map<const uint8_t *, uint32_t> line_table_;

  const uint8_t *end;
  uint8_t *start;
  uint8_t *ptr;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct instruction_t {
  instruction_e opcode;
  int32_t operand;
  const token_t *token;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct assembler_t : ast_visitor_t {

  assembler_t(ccml_t &c, asm_stream_t &stream);

  void visit(ast_program_t* n) override {
    for (ast_node_t *c : n->children) {
      dispatch(c);
    }
  }

  void visit(ast_exp_ident_t* n) override {
    // getv/getg
    emit(INS_GETV, n->name);
  }

  virtual void visit(ast_exp_const_t* n) override {
    emit(INS_CONST, n->value->val_, n->value);
  }

  void visit(ast_exp_array_t* n) override {
    // getvi/getgi
    dispatch(n->index);
    emit(INS_GETVI, 0, nullptr);
  }

  void visit(ast_exp_call_t* n) override {
    for (ast_node_t *c : n->args) {
      dispatch(c);
    }
    emit(INS_CALL, 0, n->name);
    int32_t *addr = get_fixup();
    // insert addr into map
  }

  void visit(ast_stmt_call_t *n) override {
    dispatch(n->expr);
    // discard return value
    emit(INS_POP, 1, nullptr);
  }

  void visit(ast_exp_bin_op_t* n) override {
    dispatch(n->left);
    dispatch(n->right);
    emit(tok_to_ins_(n->op->type_), n->op);
  }

  void visit(ast_exp_unary_op_t* n) override {
    dispatch(n->child);
    emit(tok_to_ins_(n->op->type_), n->op);
  }

  void visit(ast_stmt_if_t* n) override {

    // expr
    dispatch(n->expr);
    // false jump to L0 (else)
    emit(INS_FJMP, 0, n->token); // ---> LO

    // then
    for (ast_node_t *c : n->then_block) {
      dispatch(c);
    }
    // unconditional jump to end
    emit(INS_JMP, 0); // ---> L1

    // else
    // L0 <---
    for (ast_node_t *c : n->else_block) {
      dispatch(c);
    }

    // L1 <---
  }

  void visit(ast_stmt_while_t* n) override {

    // initial jump to L1 --->
    emit(INS_JMP, 0, n->token);
    int32_t *fixup = get_fixup();

    // L0 <---
    // emit the while loop body
    for (ast_node_t *c : n->body) {
      dispatch(c);
    }

    // L1 <---
    // while loop condition
    dispatch(n->expr);
    // true jump to L0 --->
    emit(INS_TJMP, 0, n->token);
  }

  void visit(ast_stmt_return_t* n) override {
    dispatch(n->expr);
    emit(INS_RET);
  }

  void visit(ast_stmt_assign_var_t* n) override {
    if (n->expr) {
      dispatch(n->expr);
      emit(INS_SETV, 0, n->name);
    }
  }

  void visit(ast_stmt_assign_array_t* n) override {
  }

  void visit(ast_decl_func_t* n) override {
    int32_t space = 0;
    if (space > 0) {
      emit(INS_LOCALS, space, n->name);
    }
    bool is_return = false;
    for (ast_node_t *c : n->body) {
      dispatch(c);
      is_return == c->is_a<ast_stmt_return_t>();
    }
    if (!is_return) {
      emit(INS_CONST, 0, nullptr);
      emit(INS_RET, space, nullptr);
    }
  }

  void visit(ast_decl_var_t* n) override {
    if (n->expr) {
      dispatch(n->expr);
      emit(INS_SETV, 0, n->name);
    }
  }

  void visit(ast_decl_array_t* n) override {
  }

  // reset any stored state
  void reset();

protected:

  instruction_e tok_to_ins_(token_e op) const {
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

  // emit into the code stream
  void emit(token_e, const token_t *t = nullptr);
  void emit(instruction_e ins, const token_t *t = nullptr);
  void emit(instruction_e ins, int32_t v, const token_t *t = nullptr);

  // return the current output head
  int32_t pos();

  // return a reference to the last instructions operand
  int32_t *get_fixup();

  // emit an instruction type
  void emit_(const instruction_t &ins);

  asm_stream_t &stream_;
  ccml_t &ccml_;
};

} // namespace ccml
