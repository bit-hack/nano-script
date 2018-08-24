#include <cassert>

#include "../lib/assembler.h"
#include "../lib/ccml.h"
#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/vm.h"

static const char *xfails[] = {
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- function
  R"(
function name)
end
  )",
  R"(
function name(
end
  )",
  R"(
function name)
end
  )",
  R"(
function name(thing thing)
end
  )",
  R"(
function ()
end
  )",
  R"(
function x(1)
end
  )",
  R"(
function x(,)
end
  )",
  R"(
function x(,hello)
end
  )",
  R"(
function x(hello,)
end
  )",
  R"(
function x(hello)
end
end
  )",
  R"(
function function(hello)
end
  )",
  R"(
function and(hello)
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- if
  R"(
function look(hello)
  end
end
  )",
  R"(
function look(hello)
  if (1)
  end
  )",
  R"(
function look(hello)
  if (1)
  else
  end
  )",
  R"(
function look()
  if (undecl)
  end
end
  )",
  R"(
function look(hello)
  var if = 1
  if (if)
  end
end
  )",
  R"(
function look(hello)
  var if = 1
  if (if)
  end
end
  )",
  R"(
function look(hello)
  else
  end
end
  )",
  R"(
function look(hello)
  if
  else
  end
end
  )",
  R"(
function look(hello)
  if (
  else
  end
end
  )",
  R"(
function look(hello)
  if (1
  else
  end
end
  )",
  R"(
function look(hello)
  if (1))
  else
  end
end
  )",
  R"(
function look(hello)
  if 1
  else
  end
end
  )",
  R"(
function look(hello)
  if 1)
  else
  end
end
  )",
  R"(
function look(hello)
  if (1) end
end
  )",
  R"(
function look(hello)
  if (1) else end
end
  )",
  R"(
function look(hello)
  if (1)
  else end
end
  )",
  R"(
function look(hello)
  if (1)
  end if (1)
  end
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- if
  R"(
function look(hello)
  while (1)
  end
  )",
  R"(
function look()
  while (undecl)
  end
end
  )",
  R"(
function look(hello)
  var while = 1
  while (while)
  end
end
  )",
  R"(
function look(hello)
  while
  end
end
  )",
  R"(
function look(hello)
  while (
  end
end
  )",
  R"(
function look(hello)
  while (1
  end
end
  )",
  R"(
function look(hello)
  while (1))
  end
end
  )",
  R"(
function look(hello)
  while 1
  end
end
  )",
  R"(
function look(hello)
  while 1)
  end
end
  )",
  R"(
function look(hello)
  while (1) end
end
  )",
  R"(
function look(hello)
  while (1)
  end while (1)
  end
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
  nullptr
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// calculate the weekday given a day, month, year
bool test_xfails() {

  int32_t fails = 0;

  for (int i = 0; xfails[i]; ++i) {

    if (i == 34) {
//      __debugbreak();
    }

    const char *source = xfails[i];

    bool failed = false;
    try {
      ccml_t ccml;
      if (!ccml.build(xfails[i])) {
        return false;
      }
    }
    catch (const ccml_error_t &error) {
      (void)error;
      failed = true;
    }
    if (!failed) {
      printf("  ! xfail test %d failed\n", i);
      ++fails;
    }
  }

  return fails == 0;
}
