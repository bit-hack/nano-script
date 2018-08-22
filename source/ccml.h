#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>


#if defined(_MSC_VER)
#define throws(X)                                                              \
  {                                                                            \
    __debugbreak();                                                            \
    throw X;                                                                   \
  }
#else
#define throws(X)                                                              \
  { throw X; }
#endif

struct lexer_t;
struct parser_t;
struct assembler_t;
struct vm_t;

struct ccml_t {

  ccml_t();
  ~ccml_t();

  lexer_t &lexer() { return *lexer_; }

  parser_t &parser() { return *parser_; }

  assembler_t &assembler() { return *assembler_; }

  vm_t &vm() { return *vm_; }

private:
  std::unique_ptr<lexer_t> lexer_;
  std::unique_ptr<parser_t> parser_;
  std::unique_ptr<assembler_t> assembler_;
  std::unique_ptr<vm_t> vm_;
  std::vector<std::string> error_;
};

typedef void (*ccml_syscall_t)(struct thread_t &thread);
