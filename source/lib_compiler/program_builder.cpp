#include "lexer.h"
#include "program_builder.h"

namespace ccml {
void program_builder_t::set_line(lexer_t &lexer, const token_t *t) {
  const uint32_t pc = head();
  if (t) {
    program_.add_line(pc, t->line_);
  }
}
} // namespace ccml
