# The CCML language

#### Overview

The CCML language and implementation are a very simple, hackable and embeddable.
A great deal of attention has been paid to keeping the implementation simple and
small.

Why have I made a new language?  Is a reasonable question, but the answer is
simply... to satisfy my own curiosity, and desire for learning new things.
CCML was born as a learning exerciser to spend a weekend implementing a compiler
and virtual machine for a very simple language.  A secondary goal was to keep
the language as simple as I possibly could, to see how useful such a basic
language could be.

Language/implementation features:
- Hand crafted recursive decent parser
- Variables can only be of signed integer type
- There are two control flow constructs; `if` and `while`
- Variables can be `global` or `local`
- Functions with arguments and return values
- External functions can be implemented in C++
- Very simple byte code (25 instructions currently)
- No external dependencies

What does CCML look like?  Check out the follow example:
```
function is_prime(x)
  var i = 2
  while (i < x / 2)
    if (x % i == 0)
      return 0
    end
    i = i + 1
  end
  return 1
end
```


#### The pipeline

CCML follows a very conventional language pipeline for which, execution can be
broken into the following broad stages.

```
source code -> lexer -> parser -> assembler -> bytecode -> virtual machine
```

----
#### More to come...
