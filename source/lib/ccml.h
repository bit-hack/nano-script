#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <array>


namespace ccml {

enum instruction_e;

struct error_manager_t;
struct error_t;

struct lexer_t;
struct token_t;
struct token_stream_t;

struct parser_t;
struct function_t;

struct asm_stream_t;
struct assembler_t;
struct disassembler_t;

struct thread_t;
struct vm_t;


struct ccml_t {

  ccml_t();
  ~ccml_t();

  // accessors
  vm_t            &vm()           { return *vm_; }
  lexer_t         &lexer()        { return *lexer_; }
  parser_t        &parser()       { return *parser_; }
  assembler_t     &assembler()    { return *assembler_; }
  disassembler_t  &disassembler() { return *disassembler_; }
  error_manager_t &errors()       { return *errors_; }

  bool build(const char *source, error_t &error);

  void reset();

  const uint8_t *code() const {
    return code_.data();
  }

private:
  friend struct vm_t;
  friend struct lexer_t;
  friend struct parser_t;
  friend struct assembler_t;
  friend struct disassembler_t;
  friend struct token_stream_t;
  friend struct error_manager_t;

  // the default code stream
  std::array<uint8_t, 1024 * 8> code_;
  std::unique_ptr<asm_stream_t> code_stream_;

  std::unique_ptr<vm_t> vm_;
  std::unique_ptr<lexer_t> lexer_;
  std::unique_ptr<parser_t> parser_;
  std::unique_ptr<assembler_t> assembler_;
  std::unique_ptr<disassembler_t> disassembler_;
  std::unique_ptr<error_manager_t> errors_;
};

typedef void(*ccml_syscall_t)(struct thread_t &thread);

} // namespace ccml
