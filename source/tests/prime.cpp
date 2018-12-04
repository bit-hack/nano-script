#include "stdint.h"

#include "../lib/ccml.h"
#include "../lib/vm.h"
#include "../lib/parser.h"
#include "../lib/codegen.h"
#include "../lib/disassembler.h"
#include "../lib/errors.h"

static const char *prime_prog = R"(
# via sieve of eratosthenes

var size = 512
var data[512]

function fill()
  var i = size
  while (i > 0)
    i = i - 1
    data[i] = i
  end
end

function mark(step)
  is_marking(step)
  var i = step + step
  while (i < size)
    data[i] = - 1
    i = i + step
  end
end

function seive()
  var i = 2
  while (i < size)
    if (data[i] >= 0)
      mark(i)
    end
    i = i + 1
  end
end

function check()
  var i = size
  while (i > 1)
    i = i - 1
    if (data[i] >= 0)
      validate(data[i])
    end
  end
end

function main()
  fill()
  seive()
  check()
end
)";

static bool is_prime(int32_t n) {
  if (n == 2 || n == 3)
    return true;
  if (n % 2 == 0 || n % 3 == 0)
    return false;
  int32_t i = 5;
  int32_t w = 2;
  while (i * i <= n) {
    if (n % i == 0) {
      return false;
    }
    i += w;
    w = 6 - w;
  }
  return true;
}

struct test_prime_t {

  static bool test_passing;
  static int32_t checks_done;

  // check that the output is in an ascending order
  static void sys_check(struct ccml::thread_t &thread) {
    ++checks_done;
    // pop prime number off stack
    const ccml::value_t value = thread.pop();
    if (!is_prime(ccml::value_to_int(value))) {
      test_passing = false;
    }
    // push dummy return value
    thread.push(ccml::value_from_int(0));
  }

  static void is_marking(struct ccml::thread_t &thread) {
    const ccml::value_t value = thread.pop();
    thread.push(ccml::value_from_int(0));
  }

  static bool run() {

    using namespace ccml;

    ccml_t ccml;
    // register syscalls
    ccml.add_function("validate", sys_check, 1);
    ccml.add_function("is_marking", is_marking, 1);
    // compile the program
    error_t error;
    if (!ccml.build(prime_prog, error)) {
      return false;
    }
//    ccml.disassembler().disasm();
    // run it
    const function_t* func = ccml.find_function("main");

    thread_t thread{ccml};
    if (!thread.prepare(*func, 0, nullptr)) {
      return false;
    }
    if (!thread.resume(1024 * 1024 * 1024, false)) {
      return false;
    }
    if (!thread.finished()) {
      return false;
    }

    const uint32_t cycle_count = thread.cycle_count();

    const value_t res = thread.return_code();
    return test_passing && !thread.has_error() && checks_done;
  }
};

bool test_prime_t::test_passing = true;
int32_t test_prime_t::checks_done = 0;

bool test_prime_1() {
  return test_prime_t::run();
}
