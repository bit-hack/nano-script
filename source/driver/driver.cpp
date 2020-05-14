#include <cstdio>

#include "../lib/ccml.h"
#include "../lib/codegen.h"
#include "../lib/disassembler.h"
#include "../lib/errors.h"
#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/vm.h"

namespace {

static inline uint32_t xorshift32() {
  static uint32_t x = 12345;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

void vm_rand(ccml::thread_t &t, int32_t) {
  // new unsigned int
  const int32_t x = (xorshift32() & 0x7fffff);
  // return value
  t.stack().push(t.gc().new_int(x));
}

void vm_getc(ccml::thread_t &t, int32_t) {
  using namespace ccml;
  const int32_t ch = getchar();
  t.stack().push_int(ch);
}

void vm_putc(ccml::thread_t &t, int32_t) {
  using namespace ccml;
  const value_t *v = t.stack().pop();
  assert(v);
  if (!v->is_a<val_type_int>()) {
    t.raise_error(thread_error_t::e_bad_argument);
  } else {
    putchar((int)(v->v));
    fflush(stdout);
  }
  t.stack().push_int(0);
}

void vm_gets(ccml::thread_t &t, int32_t) {
  using namespace ccml;
  char buffer[80];
  fgets(buffer, sizeof(buffer), stdin);
  for (size_t i = 0; i < sizeof(buffer); ++i) {
    buffer[i] = (buffer[i] == '\n') ? '\0' : buffer[i];
  }
  buffer[79] = '\0';
  t.stack().push_string(std::string{buffer});
}

void vm_puts(ccml::thread_t &t, int32_t) {
  using namespace ccml;
  value_t *s = t.stack().pop();
  assert(s);
  if (!s->is_a<val_type_string>()) {
    t.raise_error(thread_error_t::e_bad_argument);
  } else {
    printf("%s\n", s->string());
    fflush(stdout);
  }
  t.stack().push_int(0);
  return;
}

void on_error(const ccml::error_t &error) {
  fprintf(stderr, "line:%d - %s\n", error.line, error.error.c_str());
  fflush(stderr);
  exit(1);
}

void on_runtime_error(ccml::thread_t &thread, const ccml::source_manager_t &sources) {
  using namespace ccml;
  const thread_error_t err = thread.error();
  if (err == thread_error_t::e_success) {
    return;
  }
  const line_t line = thread.source_line();
  printf("runtime error %d\n", int32_t(err));
  fprintf(stderr, "%s\n", get_thread_error(thread.error()));
  printf("source line %d\n", int32_t(line.line));
  const std::string s = sources.get_line(line);
  printf("%s\n", s.c_str());

  thread.unwind();
}

void print_result(const ccml::value_t *res) {

  using namespace ccml;

  FILE *fd = stdout;
  fprintf(fd, "exit: ");
  if (!res || res->is_a<val_type_none>()) {
    fprintf(fd, "none\n");
  }
  if (res->is_a<val_type_int>()) {
    fprintf(fd, "%d", int32_t(res->v));
  }
  if (res->is_a<val_type_string>()) {
    fprintf(fd, "\"%s\"", res->string());
  }
  if (res->is_a<val_type_array>()) {
    fprintf(fd, "array");
  }
  if (res->is_a<val_type_float>()) {
    fprintf(fd, "%f", res->as_float());
  }
  if (res->is_a<val_type_func>()) {
    fprintf(fd, "function");
  }
  fprintf(fd, "\n");
}
} // namespace

int main(int argc, char **argv) {

  using namespace ccml;

  bool dump_ast = false;
  bool dump_dis = false;

  // load the source
  if (argc <= 1) {
    return -1;
  }
  source_manager_t sources;
  if (!sources.load(argv[1])) {
    fprintf(stderr, "unable to load input\n");
    return -2;
  }

  // program to compile into
  ccml::program_t program;

  // compile
  {
    // create compile stack
    ccml_t ccml{program};
    ccml.add_function("putc", vm_putc, 1);
    ccml.add_function("getc", vm_getc, 0);
    ccml.add_function("puts", vm_puts, 1);
    ccml.add_function("gets", vm_gets, 0);
    ccml.add_function("rand", vm_rand, 0);

    // build the program
    ccml::error_t error;
    if (!ccml.build(sources, error)) {
      on_error(error);
      return -3;
    }

    // dump the ast
    if (dump_ast) {
      ccml.ast().dump(stderr);
    }
  }

  // disassemble the program
  if (dump_dis) {
    disassembler_t disasm;
    disasm.dump(program, stderr);
  }

  // find the entry point
  const function_t *func = program.function_find("main");
  if (!func) {
    fprintf(stderr, "unable to locate function 'main'\n");
    return -4;
  }

  // create the vm and a thread
  ccml::vm_t vm{program};
  ccml::thread_t thread{vm};

  // call the global init function
  if (!vm.call_init()) {
    fprintf(stderr, "failed while executing @init\n");
    return -5;
  }

  // execution
  ccml::value_t *res = nullptr;
  {
    thread_error_t error = thread_error_t::e_success;
    if (!vm.call_once(*func, 0, nullptr, res, error)) {
      on_runtime_error(thread, sources);
      return -6;
    }
  }
  fflush(stdout);

  print_result(res);
  return 0;
}
