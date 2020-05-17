#include "lexer.h"
#include "program_builder.h"

namespace ccml {
void program_builder_t::set_line(lexer_t &lexer, const token_t *t) {
  const uint32_t pc = head();
  // line will either come from the token or the lexer
  const line_t line = t ? t->line_ : lexer.stream().line_number();
  store_.add_line(pc, line);
}
} // namespace ccml
