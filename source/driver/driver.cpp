#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>

#include "../lib/codegen.h"
#include "../lib/disassembler.h"
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

void vm_getc(ccml::thread_t &t) {
  using namespace ccml;
  const int64_t ch = getchar();
  t.push_int(ch);
}

void vm_putc(ccml::thread_t &t) {
  using namespace ccml;
  const value_t *v = t.pop();
  assert(v);
  if (!v->is_int()) {
    t.raise_error(thread_error_t::e_bad_argument);
  } else {
    putchar((int)(v->v));
    fflush(stdout);
  }
  t.push_int(0);
}

void vm_gets(ccml::thread_t &t) {
  using namespace ccml;
  char buffer[80];
  fgets(buffer, sizeof(buffer), stdin);
  for (int i = 0; i < sizeof(buffer); ++i) {
    buffer[i] = (buffer[i] == '\n') ? '\0' : buffer[i];
  }
  buffer[79] = '\0';
  t.push_string(std::string{buffer});
}

void vm_puts(ccml::thread_t &t) {
  using namespace ccml;
  value_t *s = t.pop();
  assert(s);
  if (!s->is_string()) {
    t.raise_error(thread_error_t::e_bad_argument);
  } else {
    printf("%s\n", s->s.c_str());
    fflush(stdout);
  }
  t.push_int(0);
  return;
}

void on_error(const ccml::error_t &error) {
  fprintf(stderr, "line:%d - %s\n", error.line, error.error.c_str());
  fflush(stderr);
  exit(1);
}

}; // namespace

int main(int argc, char **argv) {

  using namespace ccml;

  ccml_t ccml;
  ccml.add_function("putc", vm_putc, 1);
  ccml.add_function("getc", vm_getc, 0);
  ccml.add_function("puts", vm_puts, 1);
  ccml.add_function("gets", vm_gets, 0);

  if (argc <= 1)
    return -1;
  const char *source = load_file(argv[1]);
  if (!source) {
    fprintf(stderr, "unable to load input");
    return -1;
  }

  ccml::error_t error;
  if (!ccml.build(source, error)) {
    on_error(error);
    return -2;
  }

  const function_t *func = ccml.find_function("main");
  if (!func) {
    fprintf(stderr, "unable to locate function 'main'\n");
    exit(1);
  }

  value_t res;
  if (!ccml.vm().execute(*func, 0, nullptr, &res, false)) {
    fprintf(stderr, "max cycle count reached\n");
    exit(1);
  }
  fflush(stdout);

  if (res.is_int()) {
    printf("exit: %d\n", (int)(res.v));
  }
  if (res.is_string()) {
    printf("exit: %s\n", res.s.c_str());
  }
  if (res.is_none()) {
    printf("exit: none\n");
  }
  if (res.is_array()) {
    printf("exit: array\n");
  }
//  getchar();

  return 0;
}
