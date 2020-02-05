#include "codegen.h"
#include "program.h"

namespace ccml {

program_t::program_t()
  : stream_(new program_stream_t{*this}) {}

void program_t::reset() {
  line_table_.clear();
  strings_.clear();
  stream_.reset(new program_stream_t(*this));
  functions_.clear();
}

} // namespace ccml
