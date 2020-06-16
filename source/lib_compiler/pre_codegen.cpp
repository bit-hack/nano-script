#include "ast.h"
#include "errors.h"
#include "codegen.h"

namespace nano {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// compute function stack sizes and decl offsets
//
struct pregen_offset_t: public ast_visitor_t {

  pregen_offset_t(nano_t &nano)
    : errs_(nano.errors())
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
    if (n->is_syscall) {
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
// compute a function list and move into nano master object
//
struct pregen_functions_t: public ast_visitor_t {

  pregen_functions_t(nano_t &nano)
    : nano_(nano)
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

    if (n->is_syscall) {
      // add syscall to the syscall table
      program_builder_t builder(nano_.program_);
      builder.add_syscall(n->name);
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

    auto &pfunc = nano_.program_.functions();

    // set the global functions from here
    pfunc.insert(pfunc.end(), funcs_.begin(),
                            funcs_.end());
  }

  nano_t &nano_;
  function_t *f_;
  std::vector<function_t> funcs_;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// generate an init function
//
struct pregen_init_t : public ast_visitor_t {

  nano_t &nano_;
  ast_t &ast_;
  ast_decl_func_t *init_;

  pregen_init_t(nano_t &nano)
    : nano_(nano)
    , ast_(nano.ast())
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
    auto *a = ast_.alloc<ast_stmt_assign_var_t>(v->name);
    a->decl = v;
    // XXX: we might need to copy the expr instead of double using it
    a->expr = v->expr;
    init_->body->add(a);
  }

  void visit(ast_program_t *p) override {
    init_ = nano_.ast().alloc<ast_decl_func_t>("@init");
    init_->body = nano_.ast().alloc<ast_block_t>();

    for (auto *n : p->children) {
      if (auto *d = n->cast<ast_decl_var_t>()) {
        on_global(d);
      }
    }

    p->children.push_back(init_);
  }
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void run_pre_codegen(nano_t &nano) {
  auto *prog = &(nano.ast().program);

  // generate an @init function
  pregen_init_t(nano).visit(prog);

  // 
  // passes after this must NOT modify the AST
  //

  // solidify variable stack offsets
  pregen_offset_t(nano).visit(prog);
  // this must come after the offsets have been set
  pregen_functions_t(nano).visit(prog);
}

} // namespace nano
