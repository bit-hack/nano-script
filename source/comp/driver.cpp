#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>

#include "../lib/ccml.h"
#include "../lib/codegen.h"
#include "../lib/disassembler.h"
#include "../lib/errors.h"
#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/vm.h"

namespace {

const char *load_file(const char *path) {
  FILE *fd = fopen(path, "rb");
  if (!fd)
    return nullptr;

  fseek(fd, 0, SEEK_END);
  size_t size = size_t(ftell(fd));
  fseek(fd, 0, SEEK_SET);
  if (size <= 0) {
    fclose(fd);
    return nullptr;
  }

  char *source = new char[size + 1];
  if (fread(source, 1, size, fd) != size) {
    fclose(fd);
    delete[] source;
    return nullptr;
  }
  source[size] = '\0';

  fclose(fd);
  return source;
}

void on_error(const ccml::error_t &error) {
  fprintf(stderr, "line:%d - %s\n", error.line, error.error.c_str());
  fflush(stderr);
  exit(1);
}
} // namespace

FILE *fd_open(const char *base, const char *ext) {
  char buf[1024];
  sprintf(buf, "%s%s", base, ext);
  return fopen(buf, "wb");
}

void usage(const char *path) {
printf(R"(usage: %s file.ccml [-n -a -d -b]
  -n  disable codegen optimizations
  -a  emit ast
  -d  emit disassembly
  -b  emit binary
)", path);
}

int main(int argc, char **argv) {

  if (argc <= 1) {
    usage(argv[0]);
    return -1;
  }

  const char *source = load_file(argv[1]);
  if (!source) {
    fprintf(stderr, "unable to load input");
    return -1;
  }

  ccml::program_t program;

  using namespace ccml;
  ccml_t ccml{program};

  FILE *fd_ast = nullptr;
  FILE *fd_dis = nullptr;
  FILE *fd_bin = nullptr;

  for (int i = 2; i < argc; ++i) {
    const char *arg = argv[i];
    if (arg[0] != '-') {
      continue;
    }
    switch (arg[1]) {
    case 'a':
      fd_ast = fd_open(argv[1], ".ast");
      break;
    case 'd':
      fd_dis = fd_open(argv[1], ".dis");
      break;
    case 'b':
      fd_bin = fd_open(argv[1], ".bin");
      break;
    case 'n':
      ccml.optimize = false;
      break;
    }
  }

  ccml::error_t error;
  if (!ccml.build(source, error)) {
    on_error(error);
    return -2;
  }

  if (fd_ast) {
    ccml.ast().dump(fd_ast);
  }

  if (fd_dis) {
    ccml.disassembler().set_file(fd_dis);
    ccml.disassembler().disasm();
  }

  if (fd_bin) {
    const uint8_t *data = ccml.program().data();
    fwrite(data, 1, ccml.program().size(), fd_bin);
  }

  return 0;
}
