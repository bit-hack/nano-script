#pragma once

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
enum instruction_e {

  // binary operand
  //          2nd      1st
  //    push( pop() OP pop() )
  INS_ADD,
  INS_SUB,
  INS_MUL,
  INS_DIV,
  INS_MOD,
  INS_AND,
  INS_OR,

  // unary not operator
  //    push( !pop() )
  INS_NOT,

  // unary minus operator
  //    push( -pop() )
  INS_NEG,

  // compare less than
  //    push( pop() < pop() )
  INS_LT,

  // compare greater then
  INS_GT,
  INS_LEQ,
  INS_GEQ,
  INS_EQ,

  // unconditional jump
  //    pc = operand
  INS_JMP,

  // conditional jump to code offset
  //    if (pop() != 0)
  //      pc = operand
  // note: this would be more efficient in the general case to branch when the
  //       condition == 0.  we can avoid a INS_NOT instruction.
  INS_CJMP,

  // call a function
  INS_CALL,

  // return to previous frame {popping locals and args}
  INS_RET,

  // system call
  INS_SCALL,

  // pop constant:
  //    pop()
  INS_POP,

  // push constant:
  //    push( operand )
  INS_CONST,

  // reserve locals in stack frame
  INS_LOCALS,

  // accumulate local:
  //    stack[ fp + operand ] += v
  INS_ACCV,

  // get local:
  //    push( stack[ fp + operand ] )
  INS_GETV,

  // set local:
  //    v = pop()
  //    stack[ fp + operand ] = v
  INS_SETV,

  // get local indexed:
  //    i = pop()
  //    push( stack[ fp + operand + i ] )
  INS_GETVI,

  // set local indexed
  //    v = pop()
  //    i = pop()
  //    stack[ fp + operand + i ] = v
  INS_SETVI,

  // get global
  //    push( stack[ operand ] )
  INS_GETG,

  // set local:
  //    v = pop()
  //    stack[ operand ] = v
  INS_SETG,

  // get global indexed:
  //    i = pop()
  //    push( stack[ operand + i ] )
  INS_GETGI,

  // set global indexed:
  //    v = pop()
  //    i = pop()
  //    stack[ operand + i ] = v
  INS_SETGI,

  // number of instructions
  __INS_COUNT__,
};

} // namespace {}
