#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <array>
#include <cstdio>
#include <string>
#include <vector>

#include "../lib/ccml.h"
#include "../lib/vm.h"
#include "../lib/errors.h"
#include "../lib/parser.h"


std::array<char, 1024 * 64> program;


extern void print_history();


static uint32_t _seed = 12345;
uint32_t random(uint32_t max) {
    uint32_t &x = _seed;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return ((x * 0x4F6CDD1D) >> 8) % (max ? max : 1);
}


int main() {

  using namespace ccml;

  uint32_t tests = 0;
  std::vector<std::string> fails;

  for (int32_t i = 0; ; ++i) {

    std::string fname = "tests/" + std::string("test") + std::to_string(i) + ".txt";
    FILE *fd = fopen(fname.c_str(), "r");
    if (!fd) {
      break;
    }

    program.fill('\0');
    int read = fread(program.data(), 1, program.size(), fd);
    fclose(fd);
    if (read == 0) {
      continue;
    }
    program[read] = '\0';
    program[program.size() - 1] = '\0';

    // insert an error
#if 0
    program[random(read)] = random(256);
#endif

    ++tests;

    ccml_t ccml;
    // compile the program
    error_t error;
    if (!ccml.build(program.data(), error)) {
      fails.push_back(fname);
      printf("%s\n", error.error.c_str());
      printf("%d:  %s\n", error.line, ccml.lexer().get_line(error.line).c_str());
      continue;
    }

    const auto &funcs = ccml.parser().functions();
    for (const auto &func : funcs) {

      std::array<int32_t, 16> args;
      assert(func.num_args_ < args.size());
      for (uint32_t i = 0; i < func.num_args_; ++i) {
        args[i] = rand() & 0xff;
      }

      thread_t thread{ccml};
      if (!thread.prepare(func, func.num_args_, args.data())) {
        continue;
      }
      if (!thread.resume(1024 * 8, false)) {
        continue;
      }
      if (!thread.finished()) {
        continue;
      }
    }
  }

  int32_t i = 0;
  for (const auto &name : fails) {
    printf("  ! %02d/%02d  %s\n", i, int32_t(fails.size()), name.c_str());
    ++i;
  }

  printf("Ran %d tests\n", tests);
  printf("%d failed\n", int32_t(fails.size()));

  if (!fails.empty()) {
    getchar();
  }

  print_history();

  return fails.empty() ? 0 : 1;
}
