#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "ccml.h"

enum token_e {
    TOK_FUNC    ,   TOK_END     ,    TOK_IF     ,    TOK_ELSE   ,
    TOK_WHILE   ,   TOK_VAR     ,    TOK_VAL    ,    TOK_IDENT  ,
    TOK_LPAREN  ,   TOK_RPAREN  ,    TOK_COMMA  ,    TOK_EOL    ,
    TOK_ADD     ,   TOK_SUB     ,    TOK_MUL    ,    TOK_DIV    ,
    TOK_MOD     ,   TOK_AND     ,    TOK_OR     ,    TOK_NOT    ,
    TOK_ASSIGN  ,   TOK_EQ      ,    TOK_LT     ,    TOK_GT     ,
    TOK_LEQ     ,   TOK_GEQ     ,    TOK_RETURN ,    TOK_EOF    ,
};

struct token_t {

    token_t (token_e t) 
        : type_(t)
        , str_()
        , val_(0)
    {
    }

    token_t (const char * s) 
        : type_(TOK_IDENT)
        , str_(s)
        , val_(0)
    {
    }

    token_t (const std::string & s) 
        : type_(TOK_IDENT)
        , str_(s)
        , val_(0)
    {
    }

    token_t (int32_t v)
        : type_(TOK_VAL)
        , val_(v) 
    {
    }

    token_e     type_;
    std::string str_;
    int32_t     val_;
};

struct token_stream_t {

    token_stream_t()
        : index_(0)
    {}

    token_e type() {
        return stream_[index_].type_;
    }

    token_t * found(token_e type) {
        if (stream_[index_].type_ == type)
            return &stream_[index_++];
        return nullptr;
    }

    token_t * pop(token_e type) {
        if (token_t * t = found(type))
            return t;
        throws("unexpected token");
        return nullptr;
    }

    token_t * pop() {
        return &stream_[index_++];
    }

    void push(const token_t & tok) {
        stream_.push_back(tok);
    }

    void reset() {
        index_ = 0;
    }

    uint32_t index_;
    std::vector<token_t> stream_;
};
