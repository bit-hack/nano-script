# The CCML language

#### Overview

The CCML language and implementation are a very simple, hackable and embeddable.
A great deal of attention has been paid to keeping the implementation simple and
small.

Why have I made a new language?  Is a reasonable question, but the answer is
simple... to satisfy my own curiosity, and desire for learning new things.
CCML was born as a learning exercise to spend a weekend implementing a compiler
and virtual machine for a very simple language.  A secondary goal was to keep
the language as simple as I possibly could, to see how useful such a basic
language could be.

Now that the basic implementation is up and running, i'm adding new features and
extending the language, while keeping things simple but no simpler.

Language/implementation features:
- Hand crafted recursive decent parser
- Variables can only be of signed integer type
- There are two control flow constructs; `if` and `while`
- Variables can be `global` or `local`
- Supports global and local array types
- Functions take arguments and return values
- Compiler and virtual machine are embeddable
- External functions can be implemented in C++
- Very simple stack based virtual machine
- Small byte code (~27 instructions currently)
- Support for multitasking and multiple threads
- Debug information is generated during compilation
- No external dependencies
- Robust testing
- Extensively commented source code

What does CCML look like?  Check out the follow simple example:
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

CCML follows a very conventional language pipeline for which, execution can be
broken into the following broad stages.

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

----
#### The language

Broadly speaking, there are two things of interest, functions and variables.
A single variable stores a value, collections of variables can store many different values.
Functions contain code that may read the value of variables, do some processing, and change the value of variables in the process.


----
###### Comments

The first thing to mention is that some lines in a ccml program dont do anything at all.  They are ignored completely by the lanauge and are only there to help the programmer.
These are blank lines and lines known as comments.

```

# im a comment, but above and below are blank lines

```

Everything in the example given below is ignored by the language.  The use of blank lines and comments allow the programer to keep notes about their program to let others or themselves
know how it works.  We will see comments used below to hilight interesting parts in the examples.

----
###### Functions

Functions contain code to perform an number of operations, some of which may be to run other functions as well.
Within a function code will be processed a line at a time, one by one.  When one line has been fully processed, the next line below it will start to run.
As we will see, the order in which lines run can be changed using `if` and `while` statements.
An example of a function is given:

```
function a_name()
  return 1
end
```

Starting a line with the keyword `function` tells ccml that we want to define a function.  Imediately following that comes the functions name, in this case `a_name`.
After a functions name comes two parenthesis `()`, this is where we can specify the inputs to a function, known as arguments.  For now we will leave them empty, indicating that this function takes no inputs.
To mark the end of a function, on the last line we can see the `end` keyword.  All constructs in ccml that span multiple lines can be ended with the `end` keyword as we will see.
The middle line `return 1` is known as a statement, in this case a return statement.  When a statement is run, it will effect the state of the program.  We will learn more about the return statement later.

All ccml programs start by running the function named `main`.  Lets look at how one function can run another function, a process known as calling a function.

```
function func_1()
  # its empty
end

function main()
  # here we `call` func_1
  func_1()
end
```

In this case, there are two functions `func_1` and `main`.  `func_1` is empty, which is to say it can be run but it will do nothing.
The function `main` contains a statement known as a function call, in this case it will instruct `func_1` to run.
After `func_1` has run, `main` will continue to run, however in this case there are no more statements so main will finish.
When the function `main` finishes, the program will also end.

Lets look at a more complicated example of functions with inputs and outputs.

```
function func_2(an_arg)
    return an_arg
end

function main()
    func_2(3)
end
```

In this example we can see that the function `func_2` has one input named `an_arg`.
When we look at `main` we can see it calls `func_2` and gives it an input value, in this case 3.
The program starts by running `main`, the first action being to call `func_2` with one input.
At this point `main` waits while `func_2` runs.  Inside of `func_2` there is one statement, the
return statement, which in this case is returning the input `an_arg` back to whoever called it.
In this case the value 3 was passed as input to `func_2` who then returned it back to `main`.
The value 3 returned back to `main` is not used, and then the function ends and the program finishes.

----
###### The If statement



###### The While statement


###### The return statement

