#include <cstdio>

#include "program.h"

namespace {

static const uint32_t magic = (('L' << 24) | ('M' << 16) | ('C' << 8) | ('C'));

void emit(FILE *fd, int32_t val) {
  fwrite(&val, sizeof(val), 1, fd);
}

void emit(FILE *fd, const void *data, size_t size) {
  fwrite(data, 1, size, fd);
}

void emit(FILE *fd, const std::string &s) {
  emit(fd, s.size());
  emit(fd, s.data(), s.size());
}

void emit(FILE *fd, const ccml::ccml_syscall_t &s) {
  // XXX: we need to output some kind of syscall names as pointers wont work
  fwrite(&s, sizeof(s), 1, fd);
}

void emit(FILE *fd, const ccml::function_t &f) {
  emit(fd, f.name_);
  // emit syscall info
  emit(fd, f.sys_);
  emit(fd, f.sys_num_args_);
  // emit code range
  emit(fd, f.code_start_);
  emit(fd, f.code_end_);
  // emit locals
  emit(fd, f.locals_.size());
  for (const auto &i : f.locals_) {
    emit(fd, i.name_);
    emit(fd, i.offset_);
  }
  // emit args
  emit(fd, f.args_.size());
  for (const auto &i : f.args_) {
    emit(fd, i.name_);
    emit(fd, i.offset_);
  }
}

void emit(FILE *fd, const ccml::program_t::linetable_t &s) {
  emit(fd, s.size());
  for (auto &pair : s) {
    // code point
    emit(fd, pair.first);
    // source line
    emit(fd, pair.second.file);
    emit(fd, pair.second.line);
  }
}

} // namespace {}

namespace ccml {

bool program_t::serial_save(const char *path) {
  // open the output file
  FILE *fd = fopen(path, "wb");
  if (!fd) {
    return false;
  }
  // emit header
  emit(fd, magic);
#if 0
  // emit filetable
  emit(fd, file_table_.size());
  for (const auto &s : file_table_) {
    emit(fd, s);
  }
#endif
  // emit syscalls
  emit(fd, syscalls_.size());
  for (const auto &s : syscalls_) {
    emit(fd, s);
  }
  // emit functions
  emit(fd, functions_.size());
  for (const auto &f : functions_) {
    emit(fd, f);
  }
  // emit bytecode
  emit(fd, code_.size());
  emit(fd, code_.data(), code_.size());
  // emit the line table
  emit(fd, line_table_);
  // emit the string table
  emit(fd, strings_.size());
  for (const auto &s : strings_) {
    emit(fd, s);
  }
  // done
  fclose(fd);
  return false;
}

bool program_t::serial_load(const char *path) {
  (void)path;
  return false;
}


void program_t::reset() {
  syscalls_.clear();
  functions_.clear();
  code_.clear();
  line_table_.clear();
  strings_.clear();
}

const function_t *program_t::function_find(const std::string &name) const {
  for (const function_t &f : functions_) {
    if (f.name() == name) {
      return &f;
    }
  }
  return nullptr;
}

function_t *program_t::function_find(const std::string &name) {
  for (function_t &f : functions_) {
    if (f.name() == name) {
      return &f;
    }
  }
  return nullptr;
}

function_t *program_t::function_find(uint32_t pc) {
  for (function_t &f : functions_) {
    if (pc >= f.code_start_ && pc < f.code_end_) {
      return &f;
    }
  }
  return nullptr;
}

} // namespace ccml
