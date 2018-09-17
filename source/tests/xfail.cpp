#include <cassert>

#include "../lib/codegen.h"
#include "../lib/ccml.h"
#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/vm.h"
#include "../lib/errors.h"


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
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- while
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
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- var decls
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
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- globals
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
  R"(
var global = global
function look()
  return global
end
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- expression
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
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- duplicate
  R"(
function f()
  return 1
end
function f()
  return 1
end
  )",
  R"(
function look()
  var l = 1
  var l = 2
end
  )",
  R"(
var g = 1
var g = 2
  )",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- arguments
  R"(
function f(a, b, c)
end
function g()
  f(1, 2, 3, 4)
end
)",
  R"(
function f(a, b, c)
end
function g()
  f(1, 2)
end
)",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- arrays
R"(
function f()
  var my_array[
end
)",
R"(
function f()
  var my_array]
end
)",
R"(
function f()
  var my_array[]
end
)",
R"(
function f()
  var my_array[-1]
end
)",
R"(
function f()
  var my_array[0]
end
)",
R"(
function f()
  var size = 10
  var my_array[size]
end
)",
R"(
function f()
  # one element doesnt make much sense
  var my_array[1]
end
)",
R"(
function f(size)
  var my_array[size]
end
)",
R"(
function f(size)
  var my_array[2]
  my_array = 3
end
)",
R"(
function f(size)
  var x = 10
  x[0] = 10
end
)",
R"(
function f(x)
end
function g()
  var my_array[10]
  f(my_array)
end
)",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- global arrays
R"(
var my_global[12] = 1
)",
R"(
var my_global[0] = 1
)",
R"(
var my_global[]
)",
R"(
var my_global
function foo()
  my_global[1] = 123
end
)",
R"(
var my_global
function foo()
  var x = my_global[1]
end
)",
R"(
var my_global[12]
var x = my_global[1]
)",
R"(
var my_global[4]
function foo()
  my_global = 1
end
)",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- array passing
R"(
function x(input)
end
function main()
  var y[12]
  x(y)
end
)",
R"(
var y[12]
function x(input)
end
function main()
  x(y)
end
)",
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- lexer
R"(
this "is a quote mismatch
)",
R"(
this "is a quote mismatch
      on another line too"
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
    ccml::ccml_t ccml;
    ccml::error_t error;
    if (!ccml.build(source, error)) {
      failed = true;
    }
    if (!failed) {
      printf("  ! xfail test %d failed\n", i);
      printf("%s\n", source);
      ++fails;
    }
  }

  return fails == 0;
}
