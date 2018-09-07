#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <map>


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


struct code_store_t {

  code_store_t();

  const uint8_t *data() const {
    return code_.data();
  }

  asm_stream_t &stream() const {
    return *stream_;
  }

protected:
  std::array<uint8_t, 1024 * 8> code_;
  std::unique_ptr<asm_stream_t> stream_;

  // line table [PC -> Line]
  std::map<uint32_t, uint32_t> line_table_;

  struct scope_t {

    struct entry_t {
      token_t *ident;
      int32_t offset;
      bool absolute;
    };

    int32_t start, end;
    std::vector<entry_t> entries;
  };

  // scope list
  std::vector<scope_t> scope_;
};


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
    return store_.data();
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
#if 0
  std::array<uint8_t, 1024 * 8> code_;
  std::unique_ptr<asm_stream_t> code_stream_;
#else
  code_store_t store_;
#endif

  std::unique_ptr<vm_t> vm_;
  std::unique_ptr<lexer_t> lexer_;
  std::unique_ptr<parser_t> parser_;
  std::unique_ptr<assembler_t> assembler_;
  std::unique_ptr<disassembler_t> disassembler_;
  std::unique_ptr<error_manager_t> errors_;
};

typedef void(*ccml_syscall_t)(struct thread_t &thread);

} // namespace ccml
