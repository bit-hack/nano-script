#include <cstdarg>

#include "ccml.h"

#include "lexer.h"
#include "parser.h"
#include "assembler.h"
#include "vm.h"
#include "errors.h"


ccml_t::ccml_t()
    : lexer_(new lexer_t(*this))
    , parser_(new parser_t(*this))
    , assembler_(new assembler_t(*this))
    , vm_(new vm_t(*this))
    , errors_(new error_manager_t(*this))
{
}

ccml_t::~ccml_t() {
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

void ccml_t::reset() {
  lexer_->reset();
  parser_->reset();
  assembler_->reset();
  vm_->reset();
}
