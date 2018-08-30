#include "stdint.h"

#include "../lib/ccml.h"
#include "../lib/vm.h"
#include "../lib/parser.h"
#include "../lib/assembler.h"

static const char *prime_prog = R"(
# via sieve of eratosthenes

var size = 4096
var data[4096]

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
    data[i] = 0 - 1
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
  static void sys_check(struct thread_t &thread) {
    ++checks_done;
    // pop prime number off stack
    const int32_t value = thread.pop();
    if (!is_prime(value)) {
      test_passing = false;
    }
    // push dummy return value
    thread.push(0);
  }

  static void is_marking(struct thread_t &thread) {
    const int32_t value = thread.pop();
    thread.push(0);
  }

  static bool run() {
    ccml_t ccml;
    // register syscalls
    ccml.parser().add_function("validate", sys_check, 1);
    ccml.parser().add_function("is_marking", is_marking, 1);
    // compile the program
    if (!ccml.build(prime_prog)) {
      return false;
    }
    ccml.assembler().disasm();
    // run it
    const function_t* func = ccml.parser().find_function("main");

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

    const int32_t res = thread.return_code();
    return test_passing && !thread.has_error() && checks_done;
  }
};

bool test_prime_t::test_passing = true;
int32_t test_prime_t::checks_done = 0;

bool test_prime_1() {
  return test_prime_t::run();
}
