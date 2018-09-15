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
  //    push( pop() > pop() )
  INS_GT,

  // compare less than or equal to
  //    push( pop() <= pop() )
  INS_LEQ,

  // compare greater then or equal to
  //    push( pop() >= pop() )
  INS_GEQ,

  // compare equal to
  //    push( pop() == pop() )
  INS_EQ,

  // unconditional jump
  //    pc = operand
  INS_JMP,

  // conditional jump to code offset
  // note: jump when false
  //    if (pop() == 0)
  //      pc = operand
  INS_CJMP,

  // call a function
  //    push( next_pc )
  //    pc = operand
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

// return true if instruction takes an operand
bool ins_has_operand(const instruction_e ins);

// return true if execution can branch after instruction
bool ins_will_branch(const instruction_e ins);

// return true if instruction is a binary operator
bool ins_is_binary_op(const instruction_e ins);

// return true if instruction is a unary operator
bool ins_is_unary_op(const instruction_e ins);

} // namespace {}
