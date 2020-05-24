#include <cstdio>

#include "program.h"

#define TRY(X) { if (!(X)) { return false; } }

namespace {

static const uint32_t magic = (('L' << 24) | ('M' << 16) | ('C' << 8) | ('C'));

void emit(FILE *fd, int32_t val) {
  fwrite(&val, sizeof(val), 1, fd);
}

void emit(FILE *fd, const void *data, size_t size) {
  fwrite(data, 1, size, fd);
}

void emit(FILE *fd, const std::string &s) {
  emit(fd, int32_t(s.size()));
  emit(fd, s.data(), s.size());
}

void emit(FILE *fd, const nano::function_t &f) {
  // name
  emit(fd, f.name_);
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

void emit(FILE *fd, const nano::program_t::linetable_t &s) {
  emit(fd, s.size());
  for (auto &pair : s) {
    // code point
    emit(fd, pair.first);
    // source line
    emit(fd, pair.second.file);
    emit(fd, pair.second.line);
  }
}

bool consume(FILE *fd, int32_t &val) {
  return 1 == fread(&val, 4, 1, fd);
}

bool consume(FILE *fd, nano::program_t::linetable_t &s) {
  int32_t num_lines = 0;
  TRY(consume(fd, num_lines));
  for (int i = 0; i < num_lines; ++i) {
    int32_t pc = 0;
    int32_t file = 0;
    int32_t line = 0;
    TRY(consume(fd, pc));
    TRY(consume(fd, file));
    TRY(consume(fd, line));
    s[pc] = nano::line_t{file, line};
  }
  return true;
}

bool consume(FILE *fd, std::string &string) {
  int32_t size = 0;
  if (1 != fread(&size, 4, 1, fd)) {
    return false;
  }
  assert(size < 256);
  string.resize(size);
  auto &data = std::make_unique<char[]>(size + 1);
  if (fread(data.get(), 1, size, fd) != size) {
    return false;
  }
  data[size] = '\0';
  string.assign(data.get());
  return true;
}

bool consume(FILE *fd, nano::function_t &f) {
  // name
  TRY(consume(fd, f.name_));
  // emit code range
  TRY(consume(fd, f.code_start_));
  TRY(consume(fd, f.code_end_));
  // emit locals
  int32_t num_locals = 0;
  TRY(consume(fd, num_locals));
  for (int i = 0; i < num_locals; ++i) {
    f.locals_.emplace_back();
    auto &local = f.locals_.back();
    TRY(consume(fd, local.name_));
    TRY(consume(fd, local.offset_));
  }
  // emit args
  int32_t num_args = 0;
  TRY(consume(fd, num_args));
  for (int i = 0; i < num_args; ++i) {
    f.args_.emplace_back();
    auto &arg = f.args_.back();
    TRY(consume(fd, arg.name_));
    TRY(consume(fd, arg.offset_));
  }
  return true;
}

} // namespace {}

namespace nano {

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
    emit(fd, s.name_);
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
  return true;
}

bool program_t::serial_load(const char *path) {
  // open the output file
  FILE *fd = fopen(path, "rb");
  if (!fd) {
    return false;
  }
  // reset this program prior
  reset();
  // read header
  int32_t magic_no = 0;
  TRY(consume(fd, magic_no));
  TRY(magic_no == magic);
#if 0
  // emit filetable
  emit(fd, file_table_.size());
  for (const auto &s : file_table_) {
    emit(fd, s);
  }
#endif
  // read syscalls
  int32_t num_syscalls = 0;
  TRY(consume(fd, num_syscalls));
  for (int32_t i = 0; i < num_syscalls; ++i) {
    syscalls_.emplace_back();
    syscalls_.back().call_ = nullptr;
    consume(fd, syscalls_.back().name_);
  }
  // read functions
  int32_t num_functions = 0;
  TRY(consume(fd, num_functions));
  for (int32_t i = 0; i < num_functions; ++i) {
    functions_.emplace_back();
    TRY(consume(fd, functions_.back()));
  }
  // read bytecode
  int32_t code_size = 0;
  TRY(consume(fd, code_size));
  code_.resize(code_size);
  if (fread(code_.data(), 1, code_size, fd) != code_size) {
    return false;
  }
  // emit the line table
  TRY(consume(fd, line_table_));
  // emit the string table
  int32_t strings_size = 0;
  TRY(consume(fd, strings_size));
  for (int32_t i = 0; i < strings_size; ++i) {
    strings_.emplace_back();
    TRY(consume(fd, strings_.back()));
  }
  // done
  fclose(fd);
  return true;
}

void program_t::reset() {
  syscalls_.clear();
  functions_.clear();
  code_.clear();
  line_table_.clear();
  strings_.clear();
  globals_.clear();
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

function_t *program_t::function_find(int32_t pc) {
  for (function_t &f : functions_) {
    if (pc >= f.code_start_ && pc < f.code_end_) {
      return &f;
    }
  }
  return nullptr;
}

bool program_t::syscall_resolve(const std::string &name, ccml_syscall_t syscall) {
  bool res = false;
  for (auto &i : syscalls_) {
    if (i.name_ == name) {
      i.call_ = syscall;
      res = true;
    }
  }
  return res;
}

} // namespace nano
