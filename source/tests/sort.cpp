#include "stdint.h"

#include "../lib/ccml.h"
#include "../lib/vm.h"
#include "../lib/parser.h"
#include "../lib/assembler.h"

static const char *sort_prog = R"(
function main()
  var data[64]
  var i = 0

  # fill array with random data
  while (i < 64)
    data[i] = random()
    i = i + 1
  end

  # do 64 sort iterations
  i = 0
  while (i < 64)
    var j = 0
    while (j < 63)
      # sort ascending
      if (data[j] > data[j + 1])
        # swap data element
        var t = data[j]
        data[j] = data[j + 1]
        data[j + 1] = t
      end
      j = j + 1
    end
    i = i + 1
  end

  # check in the sorted array
  i = 0
  while (i < 64)
    check(data[i])
    i = i + 1
  end
end
)";

struct sort_test_t {

  static bool test_passing;

  // return random number between [0 -> 255]
  static void sys_random(struct thread_t &thread) {
    static uint32_t x = 0x12345;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    thread.push(x & 0xff);
  }

  // check that the output is in an ascending order
  static void sys_check(struct thread_t &thread) {
    static int32_t last = -1;
    const int32_t current = thread.pop();
    if (current < last) {
      test_passing = false;
    }
    last = current;
    // push dummy return value
    thread.push(0);
  }

  static bool run() {
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
    return res == 0;
  }

};

bool sort_test_t::test_passing = true;

bool test_sort_1() {
  sort_test_t test;
  return test.run();
}
