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
  // note: jump when true
  //    if (pop() != 0)
  //      pc = operand
  INS_TJMP,

  // conditional jump to code offset
  // note: jump when false
  //    if (pop() == 0)
  //      pc = operand
  INS_FJMP,

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
  INS_NEW_INT,

  // push string literal:
  //    push( string( string_table_index ) )
  INS_NEW_STR,

  // create a new array
  //    size = pop()
  //    push( new array( size ) )
  INS_NEW_ARY,

  // create a new none type
  //    push( new none )
  INS_NEW_NONE,

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

  // get array element
  //    index = pop()
  //    array = pop()
  //    push( array[ index ] )
  INS_GETA,

  // set array element
  //    value = pop()
  //    index = pop()
  //    array = pop()
  //    array[ index ] = value
  INS_SETA,

  // get global
  //    push( stack[ operand ] )
  INS_GETG,

  // set local:
  //    v = pop()
  //    stack[ operand ] = v
  INS_SETG,

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
