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

// a path compare that is insensitive to slashes and case
bool path_cmp(const char *x, const char *y) {
  for (; *x != '\0'; ++x, ++y) {
    // XXX: on linux we might want to not be case insensitive
    if (tolower(*x) == tolower(*y)) {
      continue;
    }
    if (*x == '\\' && *y == '/') {
      continue;
    }
    if (*x == '/' && *y == '\\') {
      continue;
    }
    return false;
  }
  return true;
}

} // namespace {}

namespace ccml {

void source_manager_t::imported_path(const source_t &s, std::string &in_out) {
  std::string base = base_path(s.file_path());
  in_out = base + "/" + in_out;
}

bool source_manager_t::load(const char *path) {
  for (const auto &s : sources_) {
    // file already loaded so return
    if (path_cmp(s->file_path().c_str(), path)) {
      return true;
    }
  }
  sources_.emplace_back(new source_t);
  return sources_.back()->load_from_file(path);
}

bool source_manager_t::load_from_string(const char *str) {
  sources_.emplace_back(new source_t);
  return sources_.back()->load_from_string(str);
}

} // namespace ccml
