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
