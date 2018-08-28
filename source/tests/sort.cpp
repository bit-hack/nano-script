#include "stdint.h"

#include "../lib/ccml.h"
#include "../lib/vm.h"
#include "../lib/parser.h"
#include "../lib/assembler.h"


// return random number between [0 -> 255]
static void sys_random(struct thread_t &thread) {
  static uint32_t x = 0x12345;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  thread.push(x & 0xff);
}

static void sys_check(struct thread_t &thread) {
  static int32_t last = -1;
  int32_t current = thread.pop();
  // push dummy return value
  thread.push(0);
}

static const char *sort_prog = R"(
function main()
  var data[64]
  var i = 0

  # fill array with random data
  while (i < 64)
    data[i] = random()
    i = i + 1
  end

  i = 0
  while (i < 64)
    check(data[i])
    i = i + 1
  end
end
)";

bool test_sort_1() {
  ccml_t ccml;
  // register syscalls
  ccml.parser().add_function("random", sys_random, 0);
  ccml.parser().add_function("check", sys_check, 1);
  // compile the program
  if (!ccml.build(sort_prog)) {
    return false;
  }
  ccml.assembler().disasm();
  // run it
  const int32_t func = ccml.parser().find_function("main")->pos_;
  const int32_t res = ccml.vm().execute(func, 0, nullptr, false);
  return res == 8;
}

