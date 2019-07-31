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
  const int32_t ch = getchar();
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
    printf("%s\n", s->string());
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

const char *error_to_str(const ccml::thread_error_t &err) {
  using namespace ccml;
  switch (err) {
  case thread_error_t::e_success:
    return "e_success";
  case thread_error_t::e_max_cycle_count:
    return "e_max_cycle_count";
  case thread_error_t::e_bad_getv:
    return "e_bad_getv";
  case thread_error_t::e_bad_setv:
    return "e_bad_setv";
  case thread_error_t::e_bad_num_args:
    return "e_bad_num_args";
  case thread_error_t::e_bad_syscall:
    return "e_bad_syscall";
  case thread_error_t::e_bad_opcode:
    return "e_bad_opcode";
  case thread_error_t::e_bad_set_global:
    return "e_bad_set_global";
  case thread_error_t::e_bad_get_global:
    return "e_bad_get_global";
  case thread_error_t::e_bad_pop:
    return "e_bad_pop";
  case thread_error_t::e_bad_divide_by_zero:
    return "e_bad_divide_by_zero";
  case thread_error_t::e_stack_overflow:
    return "e_stack_overflow";
  case thread_error_t::e_stack_underflow:
    return "e_stack_underflow";
  case thread_error_t::e_bad_globals_size:
    return "e_bad_globals_size";
  case thread_error_t::e_bad_array_bounds:
    return "e_bad_array_bounds";
  case thread_error_t::e_bad_type_operation:
    return "e_bad_type_operation";
  case thread_error_t::e_bad_argument:
    return "e_bad_argument";
  default:
    assert(false);
    return "";
  }
}

}; // namespace

int main(int argc, char **argv) {

  using namespace ccml;

  ccml_t ccml;
  ccml.add_function("putc", vm_putc, 1);
  ccml.add_function("getc", vm_getc, 0);
  ccml.add_function("puts", vm_puts, 1);
  ccml.add_function("gets", vm_gets, 0);

  if (argc <= 1) {
    return -1;
  }
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
    return -3;
  }

  ccml::thread_error_t err = ccml::thread_error_t::e_success;

  ccml::thread_t thread{ccml};

  if (thread.init()) {
    fprintf(stderr, "failed while executing @init\n");
  }

  if (!thread.prepare(*func, 0, nullptr)) {
    fprintf(stderr, "unable to prepare thread\n");
    return -4;
  }

  while (!thread.finished() && !thread.has_error()) {
    if (!thread.resume(1024, false)) {
      break;
    }
  }

  if (thread.has_error()) {
    fprintf(stderr, "thread error: %d\n", (int)thread.error());
    fprintf(stderr, "%s\n", error_to_str(thread.error()));
    fprintf(stderr, "line: %d\n", thread.source_line());
    return -5;
  }
  fflush(stdout);

  const ccml::value_t *res = thread.return_code();

  if (!res || res->is_none()) {
    fprintf(stdout, "exit: none\n");
  }
  if (res->is_int()) {
    fprintf(stdout, "exit: %d\n", (int32_t)(res->v));
  }
  if (res->is_string()) {
    fprintf(stdout, "exit: \"%s\"\n", res->string());
  }
  if (res->is_array()) {
    fprintf(stdout, "exit: array\n");
  }

  return 0;
}
