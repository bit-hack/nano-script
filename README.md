# The Nano Script language

#### Overview

The Nano Script language and implementation are very simple, hackable and
embeddable.  A great deal of attention has been paid to keeping the
implementation simple and small.

Why have I made a new language?  Is a reasonable question, but the answer is
simple... to satisfy my own curiosity and desire to learn new things.
Nano was born as a learning exercise to spend a weekend implementing a compiler
and virtual machine for a very simple language.  A secondary goal was to keep
the language as simple as I possibly could, to see how useful such a basic
language could be.

I managed to complete my weekend project, and now I am adding new features and
extending the language, while trying to keep things simple but no simpler.

Current language/implementation features:
- Imperative language modeled on BASIC
- Hand crafted recursive decent parser
- Dynamic type system
- Both compiler and virtual machine are embeddable
- Programs are serializable to disk
- External functions can be implemented in C++
- A simple stack based virtual machine
- Uses a copying garbage collector
- A suite of optimizations to improve generated code
- Very simple byte code format
- Support for multitasking and multiple threads
- Debug information is generated during compilation
- Supports source code breakpoints
- No external dependencies beyond the STL
- Robust testing
- Extensively commented source code

What does Nano look like?  Check out the follow simple example:
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


#### The compilation pipeline

Nano follows a very conventional compilation pipeline for which, execution can
be broken into the following broad stages:

```
 source code --.
                \ 
                 \ . lexer
                  \ . parser
                   \ . abstract syntax tree
                    \ . semantic checking
                     \ . optimizer
                      \ . code generator
                       \ . bytecode
                        \
                         '--> virtual machine
```
