//===--------------------------------------------------------------------------------------------===
// parsec.c - Main implementation of the ParseC lexer
// This source is part of ParseC
//
// Created on 2018-04-26 by Amy Parent <amy@amyparent.com>
// Copyright (c) 2018 Amy Parent <amy@amyparent.com>
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "utf8.h"
#include <math.h>
#include <assert.h>
#include <parsec/parsec.h>

// MARK: - parser helping tools



static inline codepoint_t current(const parsec* parser) {
    uint64_t remaining = parser->data_length - parser->head;
    return utf8_getCodepoint(&parser->data[parser->head], remaining);
}

static inline bool end(const parsec* parser) {
    return (parser->head + utf8_codepointSize(current(parser))) >= parser->data_length;
}

static inline codepoint_t next_char(parsec* parser) {
    codepoint_t c = current(parser);
    int8_t length = utf8_codepointSize(c);
    if(length > 0) parser->head += length;
    return current(parser);
}

typedef enum {
    STATE_SIGN,
    STATE_INTEGRAL,
    STATE_POINT,
    STATE_DECIMAL,
    STATE_E,
    STATE_ES,
    STATE_EXPONENT
} private_numstate;

static bool parse_number(parsec* parser, parsec_token* token) {
    
    codepoint_t c = current(parser);
    token->start = parser->head;
    private_numstate state = (c == '+' || c == '-') ? STATE_SIGN : STATE_INTEGRAL;
    
    for(;;) {
        c = next_char(parser);
        
        switch (state) {
        case STATE_SIGN:
            if(c >= '0' && c <= '9')                                state = STATE_INTEGRAL;
            else if(c == '.')                                       state = STATE_POINT;
            else                                                    return false;
            
        case STATE_INTEGRAL:
            if(c >= '0' && c <= '9')                                state = STATE_INTEGRAL;
            else if(c == '.')                                       state = STATE_POINT;
            else if(c == 'e' || c == 'E' || c == 'd' || c == 'D')   state = STATE_E;
            else if(utf8_isWhitespace(c))                           goto success;
            else                                                    return false;
            break;
            
        case STATE_POINT:
            if(c >= '0' && c <= '9')                                state = STATE_DECIMAL;
            else                                                    return false;
            
        case STATE_DECIMAL:
            if(c >= '0' && c <= '9')                                state = STATE_DECIMAL;
            else if(c == 'e' || c == 'E' || c == 'd' || c == 'D')   state = STATE_E;
            else if(utf8_isWhitespace(c))                           goto success;
            else                                                    return false;
            
            break;
            
        case STATE_E:
            if(c == '+' || c == '-')                                state = STATE_ES;
            else if(c >= '0' && c <= '9')                           state = STATE_EXPONENT;
            else                                                    return false;
            break;
            
        case STATE_ES:
            if(c >= '0' && c <= '9')                                state = STATE_EXPONENT;
            else                                                    return false;
            
        case STATE_EXPONENT:
            if(c >= '0' && c <= '9')                                state = STATE_EXPONENT;
            else if(utf8_isWhitespace(c))                           goto success;
            else                                                    return false;
            break;
        }
    }
    return false;
    
success:
    token->kind = state == STATE_INTEGRAL ? PARSEC_TOKEN_INT : PARSEC_TOKEN_FLOAT;
    token->length = parser->head - token->start;
    return true;
}

static bool parse_string(parsec* parser, parsec_token* token) {
    // First, we parse the initial quote
    token->start = parser->head;
    codepoint_t c = next_char(parser);
    
    for(;;) {
        // We don't accept line returns in the middle of string tokens
        if(c == '\n') return false;
        if(c == '\'') break;
        c = next_char(parser);
    }
    // consume the closing quote
    next_char(parser);
    token->kind = PARSEC_TOKEN_STRING;
    token->length = parser->head - token->start;
    return true;
}

// MARK: - 

static void skip_whitespace(parsec* parser) {
    while(utf8_isWhitespace(current(parser)) && !end(parser)) next_char(parser);
}

static void skip_line(parsec* parser) {
    while(current(parser) != '\n' && !end(parser)) next_char(parser);
}

parsec_kind token_type(codepoint_t c, char comment_char) {
    if(c == '-' || c == '+' || c == '.' || (c >= '0' && c <= '9'))  return PARSEC_TOKEN_INT;
    if(c == comment_char)                                           return PARSEC_TOKEN_COMMENT;
    if(c == '\'')                                                   return PARSEC_TOKEN_STRING;
    return PARSEC_TOKEN_INVALID;
}

// MARK: - Public API implementation

void parsec_init(parsec* status, const char* source, uint64_t length, char comment_char) {
    assert(status && "Invalid ParseC status given");
    assert(source && "Invalid source data given");
    
    status->data            = source;
    status->data_length     = length;
    status->head            = 0;
    status->next_token      = 0;
    status->comment_char    = comment_char;
}

parsec_result parsec_lex(parsec* parser, parsec_token* tokens, uint64_t token_count) {
    assert(parser && "Invalid ParseC status given");
    
    for(;;) {
        skip_whitespace(parser);
        if(end(parser)) break;
        
        // Get the next token to fill up, or fail
        if(parser->next_token >= token_count) return PARSEC_NOMEM;
        parsec_token* token = &tokens[parser->next_token++];
        
        // We save the current head for two reasons:
        //      - if the token parsing fails for any reason, we can restore (for reentrance)
        //      - if the token parsing goes well, then we have the start index
        uint64_t head = parser->head;
        
        codepoint_t c = current(parser);
        switch (token_type(c, parser->comment_char)) {
        
        case PARSEC_TOKEN_COMMENT:
            skip_line(parser);
            token->kind = PARSEC_TOKEN_COMMENT;
            token->start = head;
            token->length = (parser->head - head);
            break;
            
        case PARSEC_TOKEN_INT:
        case PARSEC_TOKEN_FLOAT:
            if(!parse_number(parser, token)) return PARSEC_INVALID;
            break;
            
        case PARSEC_TOKEN_STRING:
            if(!parse_string(parser, token)) return PARSEC_INVALID;
            break;
            
        case PARSEC_TOKEN_INVALID:
            return PARSEC_INVALID;
            break;
        }
    }
    return parser->next_token;
}

static int64_t max_power10(int64_t n) {
    int64_t p = 1;
    while(n > p) p *= 10;
    return p;
}

double parsec_str_double(const char* ptr, parsec_idx length) {
    bool sign = true;
    bool exp_sign = true;
    int64_t integral = 0;
    int64_t decimal = 0;
    int64_t exponent = 0;
    double floating = 0.0;
    const char* end = ptr + length;
    
    if(*ptr == '-' || *ptr == '+') {
        sign = *ptr == '+';
        ptr++;
    }
    
    while(*ptr >= '0' && *ptr <= '9') {
        integral = integral * 10 + (*ptr - '0');
        if(++ptr == end) { return sign ? integral : -integral; }
    }
    
    if(*ptr == '.') {
        ptr++;
        while(*ptr >= '0' && *ptr <= '9') {
            decimal = decimal * 10 + (*ptr - '0');
            if(++ptr != end) continue;
            int64_t d = max_power10(decimal);
            double val = ((double)integral + (double)decimal/(double)d);
            return sign ? val : - val;
        }
    }
    
    int64_t d = max_power10(decimal);
    floating = ((double)integral + (double)decimal/(double)d);
    
    if(*ptr == 'e' || *ptr == 'E' || *ptr == 'd' || *ptr == 'D') {
        ptr++;
        if(*ptr == '-' || *ptr == '+') {
            sign = *ptr == '+';
            ptr++;
        }
        while(*ptr >= '0' && *ptr <= '9') {
            exponent = exponent * 10 + (*ptr - '0');
            if(++ptr != end) continue;
            floating = floating * (exp_sign ? pow(10, exponent) : -pow(10, exponent));
        }
    }
    return sign ? floating : -floating;
}

int32_t parsec_str_int(const char* ptr, parsec_idx length) {
    bool sign = true;
    int32_t intval = 0;
    const char* end = ptr + length;
    
    if(*ptr == '-' || *ptr == '+') {
        sign = *ptr == '+';
        ptr++;
    }
    
    while(ptr != end) {
        intval = intval * 10 + (*ptr - '0');
        ptr++;
    }
    return sign ? intval : -intval;
}

