#include "ast.h"
#include "errors.h"
#include "codegen.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// flatten array initalizers
//
struct pregen_array_init_t: public ast_visitor_t {

  pregen_array_init_t(ccml_t &ccml)
    : ccml_(ccml)
  {
  }

  void visit(ast_program_t *a) override {
    // XXX: TODO
  }

  ccml_t &ccml_;
};

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
      assert(!n->is_arg());
      assert(!n->is_global());
      if (!n->is_const) {
        n->offset = offset_.back();
        offset_.back()++;
        stack_size_ = std::max<int32_t>(stack_size_, offset_.back());
      }
    }
  }

  void visit(ast_stmt_if_t *n) override {
    dispatch(n->expr);
    const int32_t val = offset_.back();
    if (n->then_block) {
      offset_.emplace_back(val);
      dispatch(n->then_block);
      offset_.pop_back();
    }
    if (n->else_block) {
      offset_.emplace_back(val);
      dispatch(n->else_block);
      offset_.pop_back();
    }
  }

  void visit(ast_block_t *n) override {
    const int32_t val = offset_.back();
    offset_.emplace_back(val);
    ast_visitor_t::visit(n);
    offset_.pop_back();
  }

  void visit(ast_program_t *a) override {
    for (auto &f : a->children) {
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
    if (n->syscall) {
      return;
    }
    else {
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
  }

protected:
  error_manager_t &errs_;
  int32_t global_offset_;
  int32_t stack_size_;

  std::vector<int32_t> offset_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// compute a function list and move into ccml master object
//
struct pregen_functions_t: public ast_visitor_t {

  pregen_functions_t(ccml_t &ccml)
    : ccml_(ccml)
    , f_(nullptr)
  {}

  void visit(ast_decl_var_t *n) override {
    // if we have a function
    if (f_) {
      if (n->is_local()) {
        f_->locals_.emplace_back();
        auto &l = f_->locals_.back();
        l.name_ = n->name->string();
        l.offset_ = n->offset;
      }
    }
  }

  void visit(ast_decl_func_t *n) override {

    if (n->syscall) {
      // add syscall to the syscall table
      program_builder_t builder(ccml_.program_);
      builder.add_syscall(n->syscall);
      return;
    }

    funcs_.emplace_back();
    f_ = &funcs_.back();
    f_->name_ = n->name;

    // this will be set during the codegen phase
    f_->code_start_ = 0;
    f_->code_end_ = 0;

    for (const auto &arg : n->args) {
      f_->args_.emplace_back();
      auto &a = f_->args_.back();
      a.name_ = arg->name->string();
      a.offset_ = arg->offset;
    }

    ast_visitor_t::visit(n);

    f_ = nullptr;
  }

  void visit(ast_program_t *n) override {
    ast_visitor_t::visit(n);

    auto &pfunc = ccml_.program_.functions();

    // set the global functions from here
    pfunc.insert(pfunc.end(), funcs_.begin(),
                            funcs_.end());
  }

  ccml_t &ccml_;
  function_t *f_;
  std::vector<function_t> funcs_;
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
      (void)n;
      auto *a = ast_.alloc<ast_stmt_assign_var_t>(v->name);
      a->decl = v;
      a->expr = v->expr;
      init_->body->add(a);
      return;
    }
    if (auto *n = v->expr->cast<ast_exp_lit_str_t>()) {
      (void)n;
      auto *a = ast_.alloc<ast_stmt_assign_var_t>(v->name);
      a->decl = v;
      a->expr = v->expr;
      init_->body->add(a);
      return;
    }
    if (auto *n = v->expr->cast<ast_array_init_t>()) {
      (void)n;
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
        default:
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

  // flatten array initalizers
  pregen_array_init_t(ccml).visit(prog);
  // generate an @init function
  pregen_init_t(ccml).visit(prog);

  // 
  // passes after this must NOT modify the AST
  //

  // solidify variable stack offsets
  pregen_offset_t(ccml).visit(prog);
  // this must come after the offsets have been set
  pregen_functions_t(ccml).visit(prog);
}

} // namespace ccml
