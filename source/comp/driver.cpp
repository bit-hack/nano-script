#include <cstdio>

#include "../lib/ccml.h"
#include "../lib/codegen.h"
#include "../lib/disassembler.h"
#include "../lib/errors.h"
#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/vm.h"

namespace {
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

  using namespace ccml;

  FILE *fd_ast = nullptr;
  FILE *fd_dis = nullptr;
  FILE *fd_bin = nullptr;

  if (argc <= 1) {
    usage(argv[0]);
    return -1;
  }

  // load the source
  if (argc <= 1) {
    return -1;
  }
  source_manager_t sources;
  if (!sources.load(argv[1])) {
    fprintf(stderr, "unable to load input\n");
    return -2;
  }

  bool optimize = true;

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
      optimize = false;
      break;
    }
  }

  program_t program;

  // compile
  {
    // create compile stack
    ccml_t ccml(program);
    ccml.optimize = optimize;

    // build the program
    ccml::error_t error;
    if (!ccml.build(sources, error)) {
      on_error(error);
      return -2;
    }

    // dump the ast
    if (fd_ast) {
      ccml.ast().dump(fd_ast);
    }
  }

  // disassemble the program
  if (fd_dis) {
    disassembler_t disasm;
    disasm.dump(program, fd_dis);
  }

  // dump the binary
  if (fd_bin) {
    const uint8_t *data = program.data();
    fwrite(data, 1, program.size(), fd_bin);
  }

  return 0;
}
