#include <cstdio>

#include "../lib_compiler/nano.h"
#include "../lib_compiler/codegen.h"
#include "../lib_compiler/disassembler.h"
#include "../lib_compiler/errors.h"
#include "../lib_compiler/lexer.h"
#include "../lib_compiler/parser.h"

#include "../lib_vm/vm.h"
#include "../lib_vm/thread.h"

#include "../lib_builtins/builtin.h"


namespace {

static inline uint32_t xorshift32() {
  static uint32_t x = 12345;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

void vm_rand(nano::thread_t &t, int32_t) {
  // new unsigned int
  const int32_t x = (xorshift32() & 0x7fffff);
  // return value
  t.get_stack().push(t.gc().new_int(x));
}

void vm_getc(nano::thread_t &t, int32_t) {
  using namespace nano;
  const int32_t ch = getchar();
  t.get_stack().push_int(ch);
}

void vm_putc(nano::thread_t &t, int32_t) {
  using namespace nano;
  const value_t *v = t.get_stack().pop();
  assert(v);
  if (!v->is_a<val_type_int>()) {
    t.raise_error(thread_error_t::e_bad_argument);
  } else {
    putchar((int)(v->v));
    fflush(stdout);
  }
  t.get_stack().push_int(0);
}

void vm_gets(nano::thread_t &t, int32_t) {
  using namespace nano;
  char buffer[80];
  fgets(buffer, sizeof(buffer), stdin);
  for (size_t i = 0; i < sizeof(buffer); ++i) {
    buffer[i] = (buffer[i] == '\n') ? '\0' : buffer[i];
  }
  buffer[79] = '\0';
  t.get_stack().push_string(std::string{buffer});
}

void vm_puts(nano::thread_t &t, int32_t) {
  using namespace nano;
  value_t *s = t.get_stack().pop();
  assert(s);
  if (!s->is_a<val_type_string>()) {
    t.raise_error(thread_error_t::e_bad_argument);
  } else {
    printf("%s\n", s->string());
    fflush(stdout);
  }
  t.get_stack().push_int(0);
  return;
}

void on_error(const nano::error_t &error) {
  fprintf(stderr, "file %d, line:%d - %s\n",
          error.line.file,
          error.line.line,
          error.error.c_str());
  fflush(stderr);
  exit(1);
}

void on_runtime_error(nano::thread_t &thread, const nano::source_manager_t &sources) {
  using namespace nano;
  const thread_error_t err = thread.get_error();
  if (err == thread_error_t::e_success) {
    return;
  }
  const line_t line = thread.get_source_line();
  printf("runtime error %d\n", int32_t(err));
  fprintf(stderr, "%s\n", get_thread_error(thread.get_error()));
  printf("source line %d\n", int32_t(line.line));
  const std::string s = sources.get_line(line);
  printf("%s\n", s.c_str());
}

void print_result(const nano::value_t *res) {

  using namespace nano;

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

  using namespace nano;

  bool dump_ast = false;
  bool dump_dis = false;

  // load the source
  source_manager_t sources;
  for (int i = 1; i < argc; ++i) {
    if (!sources.load(argv[i])) {
      fprintf(stderr, "unable to load input '%s'\n", argv[i]);
      return -2;
    }
  }
  if (sources.count() == 0) {
    fprintf(stderr, "no source files provided\n");
    return -1;
  }

  // program to compile into
  nano::program_t program;

  // compile
  {
    // create compile stack
    nano_t nano{program};
    builtins_register(nano);
    nano.syscall_register("putc", 1);
    nano.syscall_register("getc", 0);
    nano.syscall_register("puts", 1);
    nano.syscall_register("gets", 0);
    nano.syscall_register("rand", 0);
    nano.syscall_register("print", 1);

    // build the program
    nano::error_t error;
    if (!nano.build(sources, error)) {
      on_error(error);
      return -3;
    }

    // dump the ast
    if (dump_ast) {
      nano.ast().dump(stderr);
    }
  }

  program.syscall_resolve("putc", vm_putc);
  program.syscall_resolve("getc", vm_getc);
  program.syscall_resolve("puts", vm_puts);
  program.syscall_resolve("gets", vm_gets);
  program.syscall_resolve("rand", vm_rand);
  program.syscall_resolve("print", vm_puts);

  builtins_resolve(program);
  program.serial_save("temp.bin");

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
  nano::vm_t vm{program};

  // call the global init function
  if (!vm.call_init()) {
    fprintf(stderr, "failed while executing @init\n");
    return -5;
  }

  // execution
  nano::value_t *res = nullptr;
  {
    thread_error_t error = thread_error_t::e_success;
    if (!vm.call_once(*func, 0, nullptr, res, error)) {

        // XXX: 

//      on_runtime_error(error, sources);
      return -6;
    }
  }
  fflush(stdout);

  print_result(res);
  return 0;
}
