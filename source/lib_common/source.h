#pragma once
#include <memory>
#include <vector>
#include <cassert>

#include "file.h"
#include "types.h"

namespace ccml {

struct source_t {

  bool load_from_file(const char *filename) {
    clear();
    file_t file;
    if (!file.load(filename)) {
      return false;
    }
    if (file.size() == 0) {
      return false;
    }
    _data.reset(new char[file.size() + 1]);
    memcpy(_data.get(), file.data(), file.size());
    _data[file.size()] = '\0';
    _filename = filename;
    _gen_lines();
    return true;
  }

  bool load_from_string(const char *string) {
    clear();
    const size_t size = strlen(string);
    if (size == 0) {
      return false;
    }
    _data.reset(new char[size + 1]);
    memcpy(_data.get(), string, size);
    _data[size] = '\0';
    _filename = "unknown";
    _gen_lines();
    return true;
  }

  void clear() {
    _filename.clear();
    _data.reset();
    _lines.clear();
  }

  const char *data() const {
    return _data.get();
  }

  bool get_line(uint32_t num, std::string &out) const {
    // lines are base 1 numbers so we have to adjust
    assert(num >= 1);
    --num;
    if (num + 1 == _lines.size()) {
      out.assign(_lines[num]);
      return true;
    }
    if (num < _lines.size()) {
      out.assign(_lines[num], _lines[num + 1]);
      return true;
    }
    return false;
  }

  const std::string &file_path() const {
    return _filename;
  }

protected:
  void _gen_lines() {
    _lines.clear();
    const char *ptr = _data.get();
    _lines.push_back(ptr);
    for (; *ptr != '\0'; ++ptr) {
      switch (*ptr) {
      case '\n':
        _lines.push_back(ptr + 1);
        break;
      case '\0':
        _lines.push_back(ptr);
        return;
      }
    }
  }

  std::string _filename;
  std::unique_ptr<char[]> _data;
  std::vector<const char *> _lines;
};

struct source_manager_t {

  bool load(const char *path);

  bool load_from_string(const char *str);

  const source_t &get_source(int32_t index) const {
    assert(index >= 0 && index < int32_t(sources_.size()));
    return *sources_[index];
  }

  int32_t count() const {
    return sources_.size();
  }

  void clear() {
    sources_.clear();
  }

  const std::string get_line(line_t no) const {
    assert(no.file >= 0 && no.file < int32_t(sources_.size()));
    std::string out;
    sources_[no.file]->get_line(no.line, out);
    return out;
  }

  void imported_path(const source_t &s, std::string &in_out);

protected:
  std::vector<std::unique_ptr<source_t>> sources_;
};

} // namespace ccml
