#include <cstdio>

#include "../lib_compiler/ccml.h"
#include "../lib_compiler/codegen.h"
#include "../lib_compiler/disassembler.h"
#include "../lib_compiler/errors.h"
#include "../lib_compiler/lexer.h"
#include "../lib_compiler/parser.h"
#include "../lib_vm/vm.h"
#include "../lib_builtins/builtin.h"

namespace {
void on_error(const nano::error_t &error) {
  fprintf(stderr, "file %d, line:%d - %s\n",
          error.line.file,
          error.line.line,
          error.error.c_str());
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
printf(R"(usage: %s file.nano [-n -a -d -b]
  -n  disable codegen optimizations
  -a  emit ast
  -d  emit disassembly
  -b  emit binary
)", path);
}

int main(int argc, char **argv) {

  using namespace nano;

  FILE *fd_ast = nullptr;
  FILE *fd_dis = nullptr;
  FILE *fd_bin = nullptr;

  if (argc <= 1) {
    usage(argv[0]);
    return -1;
  }

  if (argc <= 1) {
    return -1;
  }
  source_manager_t sources;

  bool optimize = true;

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (arg[0] != '-') {
      if (!sources.load(arg)) {
        fprintf(stderr, "unable to load input '%s'\n", arg);
        return -2;
      }
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
    nano_t nano(program);
//    add_builtins(nano);
    nano.optimize = optimize;

    // build the program
    error_t error;
    if (!nano.build(sources, error)) {
      on_error(error);
      return -2;
    }

    // dump the ast
    if (fd_ast) {
      nano.ast().dump(fd_ast);
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
