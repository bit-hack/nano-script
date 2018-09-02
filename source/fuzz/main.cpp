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


int main() {

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

    ++tests;

    ccml_t ccml;
    // compile the program
    try {
      if (!ccml.build(program.data())) {
        fails.push_back(fname);
        continue;
      }
    }
    catch (const ccml_error_t &error) {
      (void)error;
      fails.push_back(fname);
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
      if (!thread.resume(1024, true)) {
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

  return fails.empty() ? 0 : 1;
}
