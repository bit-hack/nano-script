#include <algorithm>

#include "source.h"

namespace {

std::string base_path(std::string in) {
  const size_t last1 = in.rfind('/');
  const size_t last2 = in.rfind('\\');
  const size_t last = std::max(last1 == std::string::npos ? 0 : last1,
                               last2 == std::string::npos ? 0 : last2);
  in.resize(last);
  return in;
}

} // namespace {}

namespace ccml {

void source_manager_t::imported_path(const source_t &s, std::string &in_out) {
  std::string base = base_path(s.file_path());
  in_out = base + "/" + in_out;
}

} // namespace ccml
