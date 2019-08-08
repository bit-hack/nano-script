#include "ast.h"
#include "errors.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// compute function stack sizes and decl offsets
//
struct pregen_offset_t: public ast_visitor_t {

  pregen_offset_t(ccml_t &ccml)
    : errs_(ccml.errors())
    , global_offset_(0)
    , stack_size_(0)
  {
  }

  void visit(ast_decl_var_t *n) override {
    if (n->is_local()) {
      if (!n->is_const) {
        n->offset = offset_.back();
        offset_.back()++;
        stack_size_ = std::max<int32_t>(stack_size_, offset_.back());
      }
    }
  }

  void visit(ast_block_t *n) override {
    const int32_t val = offset_.back();
    offset_.emplace_back(val);
    ast_visitor_t::visit(n);
    offset_.pop_back();
  }

  void visit(ast_program_t *n) override {
    for (auto &f : n->children) {
      // collect global offsets
      if (auto *n = f->cast<ast_decl_var_t>()) {
        assert(n->is_global());
        if (!n->is_const) {
          n->offset = global_offset_;
          ++global_offset_;
        }
      }
      // visit functions
      if (auto *n = f->cast<ast_decl_func_t>()) {
        dispatch(n);
      }
    }
  }

  void visit(ast_decl_func_t *n) override {
    // set argument offsets
    int32_t i = -1;
    auto &args = n->args;
    for (auto itt = args.rbegin(); itt != args.rend(); ++itt) {
      auto *arg = (*itt)->cast<ast_decl_var_t>();
      arg->offset = i;
      --i;
    }
    // set local offsets
    offset_.clear();
    offset_.push_back(0);
    stack_size_ = 0;
    ast_visitor_t::visit(n);
    n->stack_size = stack_size_;
  }

protected:
  int32_t stack_size_;
  int32_t global_offset_;
  error_manager_t &errs_;

  std::vector<int32_t> offset_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// generate an init function
//
struct pregen_init_t : public ast_visitor_t {

  ccml_t &ccml_;
  ast_t &ast_;
  ast_decl_func_t *init_;

  pregen_init_t(ccml_t &ccml)
    : ccml_(ccml)
    , ast_(ccml.ast())
    , init_(nullptr) {
  }

  void on_global(ast_decl_var_t *v) {
    if (!v->expr) {
      return;
    }
    if (v->is_const) {
      return;
    }
    assert(v->is_global());
    if (auto *n = v->expr->cast<ast_exp_lit_var_t>()) {
      auto *a = ast_.alloc<ast_stmt_assign_var_t>(v->name);
      a->decl = v;
      a->expr = v->expr;
      init_->body->add(a);
      return;
    }
    if (auto *n = v->expr->cast<ast_exp_lit_str_t>()) {
      auto *a = ast_.alloc<ast_stmt_assign_var_t>(v->name);
      a->decl = v;
      a->expr = v->expr;
      init_->body->add(a);
      return;
    }
    if (auto *n = v->expr->cast<ast_array_init_t>()) {
      int i = 0;
      for (auto *t : n->item) {
        auto *a = ast_.alloc<ast_stmt_assign_array_t>(v->name);
        a->decl = v;
        a->index = ast_.alloc<ast_exp_lit_var_t>(i);
        switch (t->type_) {
        case TOK_INT:
          a->expr = ast_.alloc<ast_exp_lit_var_t>(t->get_int());
          break;
        case TOK_FLOAT:
          a->expr = ast_.alloc<ast_exp_lit_float_t>(t->get_float());
          break;
        case TOK_STRING:
          a->expr = ast_.alloc<ast_exp_lit_str_t>(t->string());
          break;
        case TOK_NONE:
          a->expr = ast_.alloc<ast_exp_none_t>();
          break;
        }
        init_->body->add(a);
        ++i;
      }
      return;
    }
  }

  void visit(ast_program_t *p) override {
    init_ = ccml_.ast().alloc<ast_decl_func_t>("@init");
    init_->body = ccml_.ast().alloc<ast_block_t>();

    for (auto *n : p->children) {
      if (auto *d = n->cast<ast_decl_var_t>()) {
        on_global(d);
      }
    }

    p->children.push_back(init_);
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void run_pre_codegen(ccml_t &ccml) {
  auto *prog = &(ccml.ast().program);
  pregen_init_t(ccml).visit(prog);
  // must be the last pass
  pregen_offset_t(ccml).visit(prog);
}

} // namespace ccml
