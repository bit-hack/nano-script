#include <cassert>

#include "../lib/assembler.h"
#include "../lib/ccml.h"
#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/vm.h"

#if _MSC_VER
extern "C" {
  int __stdcall IsDebuggerPresent(void);
}
#endif

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool return_value() {
  static const char *prog = R"(
function main()
  return 123
end
)";
  ccml_t ccml;
  if (!ccml.build(prog)) {
    return false;
  }
  const int32_t func = ccml.parser().find_function("main")->pos_;
  const int32_t res = ccml.vm().execute(func, 0, nullptr, false);
  return res == 123;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool return_var() {
  static const char *prog = R"(
function func_name()
  var x = 1234
  return x
end
)";
  ccml_t ccml;
  if (!ccml.build(prog)) {
    return false;
  }
  const int32_t func = ccml.parser().find_function("func_name")->pos_;
  const int32_t res = ccml.vm().execute(func, 0, nullptr, false);
  return res == 1234;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool return_arg() {
  static const char *prog = R"(
function test_arg_return(x)
  return x
end
)";
  ccml_t ccml;
  if (!ccml.build(prog)) {
    return false;
  }
  const int32_t func = ccml.parser().find_function("test_arg_return")->pos_;
  int32_t input = 7654;
  const int32_t res = ccml.vm().execute(func, 1, &input, false);
  return res == input;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_arg_passing() {
  static const char *prog = R"(
function called(x, y, z)
  var dummy = 12345
  return y + x * z
end

function main()
  return called(2, 3, 4)
end
)";
  ccml_t ccml;
  if (!ccml.build(prog)) {
    return false;
  }
  const int32_t func = ccml.parser().find_function("main")->pos_;
  ccml.assembler().disasm();
  const int32_t res = ccml.vm().execute(func, 0, nullptr, false);
  return res == 11;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_precedence() {
  static const char *prog = R"(
function main()
  return 2 + 3 * 4 + 5 * (6 + 3)
end
)";
  ccml_t ccml;
  if (!ccml.build(prog)) {
    return false;
  }
  const int32_t func = ccml.parser().find_function("main")->pos_;
  ccml.assembler().disasm();
  const int32_t res = ccml.vm().execute(func, 0, nullptr, false);
  return res == 59;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_is_prime() {
  static const char *prog = R"(
function is_prime(x)
  var i = 2
  while (i < (x / 2))
    if ((x % i) == 0)
      return 0
    end
    i = i + 1
  end
  return 1
end
)";
  ccml_t ccml;
  if (!ccml.build(prog)) {
    return false;
  }
  const int32_t func = ccml.parser().find_function("is_prime")->pos_;
  const int32_t inputs[] = {
    9973,   // prime
    9977};  // nonprime
  const int32_t res1 = ccml.vm().execute(func, 1, inputs+0, false);
  const int32_t res2 = ccml.vm().execute(func, 1, inputs+1, false);
  return res1 == 1 && res2 == 0;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_hcf() {
  static const char *prog = R"(
function hcf(a, b)
  var min = a
  var max = b
  if (a > b)
    min = b
    max = a
  end
  if ((max % min) == 0)
    return min
  else
    return hcf(max % min, min)
  end
end
)";
  ccml_t ccml;
  if (!ccml.build(prog)) {
    return false;
  }
  const int32_t func = ccml.parser().find_function("hcf")->pos_;
  const int32_t inputs[] = {
    12, 25,   // coprime
    55, 42,   // coprime
    56, 42};  // non coprime
  const int32_t res1 = ccml.vm().execute(func, 2, inputs + 0, false);
  const int32_t res2 = ccml.vm().execute(func, 2, inputs + 2, false);
  const int32_t res3 = ccml.vm().execute(func, 2, inputs + 4, false);
  return res1 == 1 && res2 == 1 && res3 != 1;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
typedef bool (*test_t)();
struct test_pair_t {
  const char *name;
  test_t func;
};

#define TEST(X) (#X), X
static const test_pair_t tests[] = {
  TEST(return_value),
  TEST(return_var),
  TEST(return_arg),
  TEST(test_arg_passing),
  TEST(test_precedence),
  TEST(test_is_prime),
  TEST(test_hcf),
  // sentinel
  nullptr, nullptr
};

static void pause() {
#if _MSC_VER
    if (IsDebuggerPresent()) {
      getchar();
    }
#endif
}

int main(const int argc, const char **args) {

  std::vector<std::string> fails;

  int32_t count = 0;

  printf("test log\n");
  printf("--------------------------------\n");

  const test_pair_t *pair = tests;
  for (; pair->name; ++pair) {

    assert(pair->name && pair->func);

    bool result = false;
    try {
      result = pair->func();
    }
    catch (const ccml_error_t &error) {
      (void)error;
      result = false;
    }

    if (!result) {
      fails.push_back(pair->name);
    }
    ++count;

    printf("%c %s\n", result ? '.' : 'F',  pair->name);
  }

  printf("--------------------------------\n");
  printf("%d tests, %d failures\n", count, int(fails.size()));
  for (const auto &name : fails) {
    printf("  ! %s\n", name.c_str());
  }

  if (!fails.empty()) {
    pause();
  }

  return int32_t(fails.size());
}
