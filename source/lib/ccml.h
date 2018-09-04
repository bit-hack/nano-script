#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>


namespace ccml {

struct error_manager_t;
struct error_t;

struct lexer_t;
struct token_t;
struct token_stream_t;

struct parser_t;
struct function_t;

struct assembler_t;

struct thread_t;
struct vm_t;


struct ccml_t {

  ccml_t();
  ~ccml_t();

  lexer_t &lexer() {
    return *lexer_;
  }

  parser_t &parser() {
    return *parser_;
  }

  assembler_t &assembler() {
    return *assembler_;
  }

  error_manager_t &errors() {
    return *errors_;
  }

  vm_t &vm() {
    return *vm_;
  }

  bool build(const char *source, error_t &error);

  void reset();

private:
  friend struct lexer_t;
  friend struct parser_t;
  friend struct assembler_t;
  friend struct vm_t;
  friend struct token_stream_t;
  friend struct error_manager_t;

  std::unique_ptr<lexer_t> lexer_;
  std::unique_ptr<parser_t> parser_;
  std::unique_ptr<assembler_t> assembler_;
  std::unique_ptr<vm_t> vm_;
  std::unique_ptr<error_manager_t> errors_;
};

typedef void(*ccml_syscall_t)(struct thread_t &thread);

} // namespace ccml
