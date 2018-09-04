#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>

#include "../lib/assembler.h"
#include "../lib/ccml.h"
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
  long size = ftell(fd);
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

void vm_getc(thread_t &t) {
  const int32_t ch = getchar();
  t.push(ch);
}

void vm_putc(thread_t &t) {
  const int32_t v = t.pop();
  putchar(v);
  t.push(0);
}

void on_error(const ccml_error_t &error) {
  fprintf(stderr, "line:%d - %s\n", error.line, error.error.c_str());
  fflush(stderr);
  exit(1);
}

}; // namespace

int main(int argc, char **argv) {

  ccml_t ccml;
  ccml.parser().add_function("putc", vm_putc, 1);
  ccml.parser().add_function("getc", vm_getc, 0);

  if (argc <= 1)
    return -1;
  const char *source = load_file(argv[1]);
  if (!source) {
    printf("unable to load input");
    return -1;
  }
  if (!ccml.lexer().lex(source))
    return -1;
  delete[] source;

  ccml_error_t error;
  if (!ccml.parser().parse(error)) {
    on_error(error);
    return -2;
  }
  ccml.assembler().disasm();

  const function_t *func = ccml.parser().find_function("main");
  printf("entry point: %d\n", func->pos_);

  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    fprintf(stderr, "max cycle count reached\n");
    exit(1);
  }
  fflush(stdout);

  printf("exit: %d\n", res);
  getchar();

  return 0;
}
