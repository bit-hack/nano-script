#pragma once
#include "ccml.h"
#include "token.h"

struct lexer_t {

    ccml_t & ccml_;

    lexer_t(ccml_t & c)
        : ccml_(c)
    {}

    bool lex(const char * source);

    token_stream_t stream_;

protected:
    void push(token_e);
    void push_ident(const char * start, const char * end);
    void push_val(const char * start, const char * end);
};
