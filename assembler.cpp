#include <cstring>
#include "assembler.h"

assembler_t::assembler_t(ccml_t & c)
    : ccml_(c)
    , head_(0)
{}

int32_t * assembler_t::cjmp() {
    emit(INS_JMP, 0);
    return (int32_t*)(code_ + (head_ - 4));
}

void assembler_t::cjmp(int32_t pos) {
    emit(INS_JMP, pos);
}

void assembler_t::print(const char * str) {
    printf("%s\n", str);
}

void assembler_t::print(const char * str, int32_t v) {
    printf("%s %d\n", str, v);
}

void assembler_t::write8(uint8_t v) {
    code_[head_++] = v;
}

void assembler_t::write32(int32_t v) {
    memcpy(code_ + head_, &v, sizeof(v));
    head_+=4;
}

void assembler_t::emit(instruction_e ins) {
    switch (ins) {
    case(INS_ADD) :
    case(INS_SUB) :
    case(INS_MUL) :
    case(INS_DIV) :
    case(INS_MOD) :
    case(INS_AND) :
    case(INS_OR)  :
    case(INS_NOT) :
    case(INS_LT)  :
    case(INS_GT)  :
    case(INS_LEQ) :
    case(INS_GEQ) :
    case(INS_EQ)  :
    case(INS_NOP) : write8(ins);
                    break;
    default:
        throws("unknown instruction");
    }
}

void assembler_t::emit(instruction_e ins, int32_t v) {
    switch (ins) {
    case(INS_JMP  ):
    case(INS_CALL ):
    case(INS_RET  ):
    case(INS_POP  ):
    case(INS_CONST):
    case(INS_GETV ):
    case(INS_SETV ): write8(ins);
                     write32(v);
                     break;
    default:
        throws("unknown instruction");
    }
}

void assembler_t::emit(ccml_syscall_t sys) {
    write8(INS_SCALL);
    memcpy(code_+head_, &sys, sizeof(ccml_syscall_t));
    head_ += sizeof(ccml_syscall_t);
}

int32_t assembler_t::pos() const {
    return head_;
}

void assembler_t::disasm() {

    for (uint32_t i = 0; i < head_;) {
        printf("%04d ", i);

        uint8_t op = code_[i];
        i += 1;

        switch (op) {
        case(INS_ADD) : print("INS_ADD   "); continue;
        case(INS_SUB) : print("INS_SUB   "); continue;
        case(INS_MUL) : print("INS_MUL   "); continue;
        case(INS_DIV) : print("INS_DIV   "); continue;
        case(INS_MOD) : print("INS_MOD   "); continue;
        case(INS_AND) : print("INS_AND   "); continue;
        case(INS_OR)  : print("INS_OR    "); continue;
        case(INS_NOT) : print("INS_NOT   "); continue;
        case(INS_LT)  : print("INS_LT    "); continue;
        case(INS_GT)  : print("INS_GT    "); continue;
        case(INS_LEQ) : print("INS_LEQ   "); continue;
        case(INS_GEQ) : print("INS_GEQ   "); continue;
        case(INS_EQ)  : print("INS_EQ    "); continue;
        case(INS_NOP) : print("INS_NOP   "); continue;
        }

        if (op == INS_SCALL) {
            void * ptr = 0;
            memcpy(&ptr, code_ + i, sizeof(void*));
            i += sizeof(void*);
            printf("INS_SCALL  %p\n", ptr);
            continue;
        }

        int32_t val = *(int32_t*)(code_ + i);
        i += 4;

        switch (op) {
        case(INS_JMP  ): print("INS_CJMP  ", val); continue;
        case(INS_CALL ): print("INS_CALL  ", val); continue;
        case(INS_RET  ): print("INS_RET   ", val); continue;
        case(INS_POP  ): print("INS_POP   ", val); continue;
        case(INS_CONST): print("INS_CONST ", val); continue;
        case(INS_GETV ): print("INS_GETV  ", val); continue;
        case(INS_SETV ): print("INS_SETV  ", val); continue;
        }


        throws("unknown opcode");
    }
}

void assembler_t::emit(token_e tok) {
    switch (tok) {
    case(TOK_ADD): emit(INS_ADD); break;
    case(TOK_SUB): emit(INS_SUB); break;
    case(TOK_MUL): emit(INS_MUL); break;
    case(TOK_DIV): emit(INS_DIV); break;
    case(TOK_MOD): emit(INS_MOD); break;
    case(TOK_AND): emit(INS_AND); break;
    case(TOK_OR ): emit(INS_OR ); break;
    case(TOK_NOT): emit(INS_NOT); break;
    case(TOK_EQ ): emit(INS_EQ ); break;
    case(TOK_LT ): emit(INS_LT ); break;
    case(TOK_GT ): emit(INS_GT ); break;
    case(TOK_LEQ): emit(INS_LEQ); break;
    case(TOK_GEQ): emit(INS_GEQ); break;
    default:
        throws("cant emit token type");
    }
}
