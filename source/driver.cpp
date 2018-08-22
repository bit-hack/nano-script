#include <cstdio>

#include "assembler.h"
#include "ccml.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

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

}; // namespace

int main(int argc, char **argv) {

  ccml_t ccml;
  ccml.parser().add_function("putc", vm_putc);
  ccml.parser().add_function("getc", vm_getc);

  try {
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

    if (!ccml.parser().parse())
      return -2;
    ccml.assembler().disasm();

    int32_t func = ccml.parser().find_function("main")->pos_;
    printf("entry point: %d\n", func);

    int32_t res = ccml.vm().execute(func, 0, nullptr);
    fflush(stdout);

    printf("exit: %d\n", res);
    getchar();

  } catch (const char *msg) {
    printf("%s", msg);
    exit(1);
  }

  return 0;
}
