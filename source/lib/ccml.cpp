#include <cstdarg>

#include "ccml.h"

#include "lexer.h"
#include "parser.h"
#include "assembler.h"
#include "vm.h"

ccml_t::ccml_t()
    : lexer_(new lexer_t(*this))
    , parser_(new parser_t(*this))
    , assembler_(new assembler_t(*this))
    , vm_(new vm_t(*this))
{
}

ccml_t::~ccml_t() {
}

void ccml_t::on_error_(uint32_t line, const char *fmt, ...) {
  // generate the error message
  char buffer[1024] = {'\0'};
  va_list va;
  va_start(va, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, va);
  buffer[sizeof(buffer) - 1] = '\0';
  va_end(va);
  // throw an error
  ccml_error_t error{buffer, line};
  throw error;
}

bool ccml_t::build(const char *source) {
  if (!lexer_->lex(source)) {
    return false;
  }
  if (!parser_->parse()) {
    return false;
  }
  return true;
}
