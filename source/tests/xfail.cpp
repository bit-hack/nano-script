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
function x()
end
end
  )",
  R"(
function function()
end
  )",
  R"(
function and()
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- if
  R"(
function look()
  end
end
  )",
  R"(
function look()
  if (1)
  end
  )",
  R"(
function look()
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
function look()
  var if = 1
  if (if)
  end
end
  )",
  R"(
function look()
  var if = 1
  if (if)
  end
end
  )",
  R"(
function look()
  else
  end
end
  )",
  R"(
function look()
  if
  else
  end
end
  )",
  R"(
function look()
  if (
  else
  end
end
  )",
  R"(
function look()
  if (1
  else
  end
end
  )",
  R"(
function look()
  if (1))
  else
  end
end
  )",
  R"(
function look()
  if 1
  else
  end
end
  )",
  R"(
function look()
  if 1)
  else
  end
end
  )",
  R"(
function look()
  if (1) end
end
  )",
  R"(
function look()
  if (1) else end
end
  )",
  R"(
function look()
  if (1)
  else end
end
  )",
  R"(
function look()
  if (1)
  end if (1)
  end
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- if
  R"(
function look()
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
function look()
  var while = 1
  while (while)
  end
end
  )",
  R"(
function look()
  while
  end
end
  )",
  R"(
function look()
  while (
  end
end
  )",
  R"(
function look()
  while (1
  end
end
  )",
  R"(
function look()
  while (1))
  end
end
  )",
  R"(
function look()
  while 1
  end
end
  )",
  R"(
function look()
  while 1)
  end
end
  )",
  R"(
function look()
  while (1) end
end
  )",
  R"(
function look()
  while (1)
  end while (1)
  end
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
  R"(
function look()
  var var = 1
end
  )",
  R"(
function look()
  var = 1
end
  )",
  R"(
function look()
  var x = y
end
  )",
  R"(
function look()
  var x = 1 + y
end
  )",
  R"(
function look()
  # cant reference something before its fully declared
  var x = x
end
  )",
  R"(
function look()
  var x = y
  var y
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
  R"(
var global = 1 + 1
function look()
  return global
end
  )",
  R"(
function look()
  return global
end
var global
  )",
  R"(
function look()
  return look
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
  R"(
function look()
  var x = 1 + 2 3
end
  )",
  R"(
function look()
  var x = 1 + + 2
end
var global
  )",
  R"(
function look()
  var x = 1 + 2 ) + 3
end
  )",
  R"(
function look()
  var x = 1 ( + 2 ) + 3
end
  )",
  R"(
function look()
  var x = 1 + 2 + 3 +
end
  )",
  R"(
function look()
  var x = 1 + 2 + ( 3 + )
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

    const char *source = xfails[i];

    if (i == 44) {
//      __debugbreak();
    }

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
