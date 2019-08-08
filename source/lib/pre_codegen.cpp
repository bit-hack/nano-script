#include "ast.h"
#include "errors.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// Compute function stack sizes and decl offsets
//
struct sema_offset_t: public ast_visitor_t {

  sema_offset_t(ccml_t &ccml)
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
      if (auto *n = f->cast<ast_decl_var_t>()) {
        // dont dispatch local vars
//      dispatch(n);
        if (!n->is_const) {
          n->offset = global_offset_;
          ++global_offset_;
        }
      }
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
void run_pre_codegen(ccml_t &ccml) {
  auto *prog = &(ccml.ast().program);
  sema_offset_t(ccml).visit(prog);
}

} // namespace ccml
