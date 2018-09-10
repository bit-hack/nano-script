#include <cassert>

#include "../lib/assembler.h"
#include "../lib/disassembler.h"
#include "../lib/ccml.h"
#include "../lib/errors.h"
#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/vm.h"

#if _MSC_VER
extern "C" {
int __stdcall IsDebuggerPresent(void);
}
#endif

using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool return_value() {
  static const char *prog = R"(
function main()
  return 123
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
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
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("func_name");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
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
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("test_arg_return");
  int32_t input = 7654;
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 1, &input, &res)) {
    return false;
  }
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
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 11;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_precedence_1() {
  static const char *prog = R"(
function main()
  return 2 + 3 * 4 + 5 * (6 + 3)
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 59;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_precedence_2() {
  static const char *prog = R"(
function main()
  return 2 * 3 > 4
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 1;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_precedence_3() {
  static const char *prog = R"(
function main()
  return 1 + 1 * 2
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 3;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_precedence_4() {
  static const char *prog = R"(
function main()
  return 1 + 2 > 2 and 2 * 5 == 10
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 1;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_precedence_5() {
  static const char *prog = R"(
function main()
  return not (1 + 2 > 2 and 2 * 5 == 10)
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 0;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_precedence_6() {
  static const char *prog = R"(
function main()
  # check order of divides is correct
  # should evaluate as `(8 / 4) / 2 == 1`
  # not as             `8 / (4 / 2) == 4`
  return 8 / 4 / 2
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 1;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_precedence_7() {
  static const char *prog = R"(
function main()
  # check order of subtracts is correct
  # should evaluate as `(12 - 7) - 5 == 0`
  # not as             `12- (7 - 5) == 10`
  return 12 - 7 - 5
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 0;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_global_1() {
  static const char *prog = R"(
var global = 1234

function func_b()
  return global
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("func_b");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 1234;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_global_2() {
  static const char *prog = R"(
var global = 1234

function func_a()
  global = 987
end

function func_b()
  func_a()
  return global
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("func_b");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 987;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_global_3() {
  static const char *prog = R"(
var global = 0

function recurse( count )
  if (not count == 0)
    global = global + 1
    return recurse(count-1)
  else
    return global
  end
end

function driver()
  return recurse(15)
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("driver");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 15;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_global_4() {
  static const char *prog = R"(
var size = 128
var data[128]

function driver()
  var i = size
  return i
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("driver");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 128;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_scope() {
  static const char *prog = R"(
function scope(flag)
  if (flag)
    var x = 1234
  end
  return x
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    // XXX: make warning "variable cant be accessed from this scope"
    return true;
  }
  return false;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// integer square route
static bool test_sqrt() {
  static const char *prog = R"(
function next(n, i)
  return (n + i / n) / 2
end

function abs(i)
  if (i >= 0)
    return i
  else
    return 0 - i
  end
end

function sqrt(number)
  var n = 1
  var n1 = next(n, number)
  while (abs(n1 - n) > 1)
    n  = n1
    n1 = next(n, number)
  end
  while (n1 * n1 > number)
    n1 = n1 - 1
  end
  return n1
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("sqrt");
  int32_t input[] = {1234};
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 1, input + 0, &res)) {
    return false;
  }
  return res == 35;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// prime number test
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
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }

  const function_t *func = ccml.parser().find_function("is_prime");
  const int32_t     primes[] = {13, 17, 19, 23, 29, 9973, 0}; // prime
  const int32_t non_primes[] = {12, 15, 9, 21, 33, 9977, 0}; // nonprime

  for (int i = 0; primes[i]; ++i) {
    int32_t res = 0;
    if (!ccml.vm().execute(*func, 1, primes + i, &res)) {
      return false;
    }
    if (res != 1) {
      return false;
    }
  }
  for (int i = 0; non_primes[i]; ++i) {
    int32_t res = 1;
    if (!ccml.vm().execute(*func, 1, non_primes + 1, &res)) {
      return false;
    }
    if (res != 0) {
      return false;
    }
  }
  return true;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// check for coprime
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
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("hcf");
  const int32_t inputs[] = {12, 25,  // coprime
                            55, 42,  // coprime
                            56, 42}; // non coprime
  int32_t res1, res2, res3;
  if (!ccml.vm().execute(*func, 2, inputs + 0, &res1)) {
    return false;
  }
  if (!ccml.vm().execute(*func, 2, inputs + 2, &res2)) {
    return false;
  }
  if (!ccml.vm().execute(*func, 2, inputs + 4, &res3)) {
    return false;
  }
  return res1 == 1 && res2 == 1 && res3 != 1;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// fibbonacci generator
static bool test_fib() {
  static const char *prog = R"(
function fib(count)
  var a = 0
  var b = 1
  while (count >= 2)
    var c = a + b
    a = b
    b = c
    count = count - 1
  end
  return b
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("fib");
  const int32_t inputs[] = {9};
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 1, inputs, &res)) {
    return false;
  }
  return res == 34;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// greatest common divisor
static bool test_gcd() {
  static const char *prog = R"(
function main(a, b)
  while (not a == b)
    if (a > b)
      a = a - b
    else
      b = b - a
    end
  end
  return a
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  const int32_t inputs[] = {81, 153};
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 2, inputs, &res)) {
    return false;
  }
  return res == 9;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// very silly triangle number test
static bool test_triangle() {
  static const char *prog = R"(
function main(a)
  var x = a
  var y
  var z = 0
  while (not x == 0)
    y = x
    while (not y == 0)
      z = z + 1
      y = y - 1
    end
    x = x - 1
  end
  return z
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  const int32_t inputs[] = {3};
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 1, inputs, &res)) {
    return false;
  }
  return res == 6;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// calculate the weekday given a day, month, year
static bool test_weekday() {
  static const char *prog = R"(
# - 0 "Sunday"
# - 1 "Monday"
# - 2 "Tuesday"
# - 3 "Wednesday"
# - 4 "Thursday"
# - 5 "Friday"
# - 6 "Saturday"
function weekday(day, month, year)
  var a = 14-month
  a = a / 12
  var y = year-a
  var m = month+(12*a)-2
  var d = (day+y+(y/4)-(y/100)+(y/400)+((31*m)/12)) % 7
  return d
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("weekday");
  const int32_t inputs[] = {23,    // day
                            8,     // month
                            2018}; // year
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 3, inputs, &res)) {
    return false;
  }
  return res == 4; // thursday
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_lexer_1() {
  // test fix for bug where this would be lex'd as two tokens 'return' and
  // '1234'
  static const char *prog = "return1234";
  ccml_t ccml;
  if (!ccml.lexer().lex(prog)) {
    return false;
  }
  auto &stream = ccml.lexer().stream_;
  const token_t *tok = stream.pop();
  return (tok->type_ == TOK_IDENT) && (tok->str_ == prog);
}

static bool test_lexer_2() {
  static const char *prog = "\"quoted string here\"";
  ccml_t ccml;
  if (!ccml.lexer().lex(prog)) {
    return false;
  }
  auto &stream = ccml.lexer().stream_;
  const token_t *tok = stream.pop();
  return tok->type_ == TOK_STRING && tok->str_ == "quoted string here";
}

static bool test_lexer_3() {
  static const char *prog = "\"\"";
  ccml_t ccml;
  if (!ccml.lexer().lex(prog)) {
    return false;
  }
  auto &stream = ccml.lexer().stream_;
  const token_t *tok = stream.pop();
  return (tok->type_ == TOK_STRING) && tok->str_.empty();
}

static bool test_lexer_4() {
  static const char *prog = "a\"b\"c\"d\"e";
  ccml_t ccml;
  if (!ccml.lexer().lex(prog)) {
    return false;
  }
  auto &stream = ccml.lexer().stream_;
  for (int32_t i = 0; i < 5; ++i) {
    const token_t *tok = stream.pop();
    if (tok->str_[0] != 'a' + i) {
      return false;
    }
    if ((i % 2) == 0) {
      if (tok->type_ != TOK_IDENT)
        return false;
    } else {
      if (tok->type_ != TOK_STRING)
        return false;
    }
  }
  return true;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_array_1() {
  static const char *prog = R"(
function main()
  var my_array[4]
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 0;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_array_2() {
  static const char *prog = R"(
function main()
  var my_array[4]
  my_array[2] = 1234
  return my_array[2]
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 1234;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_array_3() {
  static const char *prog = R"(
function main()
  var my_array[10]
  var i = 0
  while (i < 10)
    my_array[i] = i
    i = i + 1
  end
  return my_array[8]
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 8;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_array_4() {
  static const char *prog = R"(
var array[8]

function foo()
  array[1] = 4
  array[4] = 9
end

function main()
  foo()
  return array[1] + array[4]
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 13;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_array_5() {
  static const char *prog = R"(
var array[8]

function foo()
  var i = 0
  while (i < 8)
    array[i] = i * i
    i = i + 1
  end
end

function main()
  foo()
  var sum = 0
  var i = 0
  while (i < 8)
    sum = sum + array[i]
    i = i + 1
  end
  return sum
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 140;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_array_6() {
  static const char *prog = R"(
var array[8]

function foo()
  array[0] = 6
  array[1] = 1
  array[2] = 4
  array[3] = 3
  array[4] = 7
  array[5] = 0
  array[6] = 5
  array[7] = 2
end

function main()
  foo()
  var t[8]

  var i = 0
  while (i < 8)
    t[i] = i
    i = i + 1
  end

  var sum = 0
  i = 0
  while (i < 8)
    sum = sum + t[ array[i] ]
    i = i + 1
  end
  return sum
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 28;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_ret_operand() {
  // return operand should match INS_LOCALS operand
  static const char *prog = R"(
function main(a, b)
  var my_array[10]
  var i = 1
  if (i == 2)
    if (i == 1)
      var thingy[2]
      return 1
    else
      var x
    end
    var y
    return a
  end
  return my_array[8]
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  // note:
  //    INS_LOCALS shoud == 13
  //    INS_RET should == 15

  const function_t *func = ccml.parser().find_function("main");

  int32_t inputs[] = {1, 2};

  int32_t res = 0;
  if (!ccml.vm().execute(*func, 2, inputs, &res)) {
    return false;
  }

  // TODO: check the generated ASM
  return true;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_neg_1() {
  static const char *prog = R"(
function main()
  return -1
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == -1;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_neg_2() {
  static const char *prog = R"(
function main()
  return -3 - -5
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 2;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_neg_3() {
  static const char *prog = R"(
function x()
  return 3
end
function main()
  return -x() - 2
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == -5;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_neg_4() {
  static const char *prog = R"(
function x()
  return 3
end
function main()
  return - ( 3 + 4 )
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == -7;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
static bool test_acc_1() {
  static const char *prog = R"(
function main()
  var x = 1
  x += 2
  return x
end
)";
  ccml_t ccml;
  error_t error;
  if (!ccml.build(prog, error)) {
    return false;
  }
  ccml.disassembler().disasm();
  const function_t *func = ccml.parser().find_function("main");
  int32_t res = 0;
  if (!ccml.vm().execute(*func, 0, nullptr, &res)) {
    return false;
  }
  return res == 3;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
typedef bool (*test_t)();
struct test_pair_t {
  const char *name;
  test_t func;
};

bool test_xfails();
bool test_sort_1();
bool test_prime_1();
bool test_debug_1();

#define TEST(X) (#X), X
static const test_pair_t tests[] = {
    TEST(return_value),
    TEST(return_var),
    TEST(return_arg),
    TEST(test_arg_passing),
    TEST(test_precedence_1),
    TEST(test_precedence_2),
    TEST(test_precedence_3),
    TEST(test_precedence_4),
    TEST(test_precedence_5),
    TEST(test_precedence_6),
    TEST(test_precedence_7),
    TEST(test_global_1),
    TEST(test_global_2),
    TEST(test_global_3),
    TEST(test_global_4),
    TEST(test_scope),
    TEST(test_sqrt),
    TEST(test_is_prime),
    TEST(test_hcf),
    TEST(test_fib),
    TEST(test_gcd),
    TEST(test_triangle),
    TEST(test_weekday),
    TEST(test_xfails),
    TEST(test_lexer_1),
    TEST(test_lexer_2),
    TEST(test_lexer_3),
    TEST(test_lexer_4),
    TEST(test_array_1),
    TEST(test_array_2),
    TEST(test_array_3),
    TEST(test_array_4),
    TEST(test_array_5),
    TEST(test_array_6),
    TEST(test_ret_operand),
    TEST(test_sort_1),
    TEST(test_prime_1),
    TEST(test_debug_1),
    TEST(test_neg_1),
    TEST(test_neg_2),
    TEST(test_neg_3),
    TEST(test_neg_4),
    TEST(test_acc_1),
    // sentinel
    nullptr, nullptr};

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
    } catch (const error_t &error) {
      (void)error;
      result = false;
    }

    if (!result) {
      fails.push_back(pair->name);
    }
    ++count;

    printf("%c %s\n", result ? '.' : 'F', pair->name);
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
