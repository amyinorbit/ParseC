//===--------------------------------------------------------------------------------------------===
// parsec.h - A simple C API for lexing files
// This source is part of ParseC
//
// Created on 2018-04-26 by Amy Parent <amy@amyparent.com>
// Copyright (c) 2018 Amy Parent <amy@amyparent.com>
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#ifndef _PARSEC_H_
#define _PARSEC_H_

#include <stdint.h>

typedef struct  parsec_token_s  parsec_token;
typedef struct  parsec_s        parsec;
typedef enum    parsec_result_e parsec_result;
typedef enum    parsec_kind_e   parsec_kind;

typedef uint32_t                parsec_idx;

enum parsec_kind_e {
    PARSEC_TOKEN_INVALID    = -1,
    PARSEC_TOKEN_STRING,
    PARSEC_TOKEN_INT,
    PARSEC_TOKEN_FLOAT,
    PARSEC_TOKEN_COMMENT,
};

enum parsec_result_e {
    PARSEC_SUCCESS          =  0,
    PARSEC_NOMEM            = -1,
    PARSEC_INVALID          = -2
};

struct parsec_token_s {
    parsec_kind kind;
    parsec_idx  start;
    parsec_idx  length;
};

struct parsec_s {
    char        comment_char;
    const char* data;
    parsec_idx  data_length;
    parsec_idx  head;
    parsec_idx  next_token;
};


void parsec_init(parsec* status, const char* source, uint64_t length, char comment_char);
parsec_result parsec_lex(parsec* status, parsec_token* tokens, uint64_t token_count);
double parsec_str_double(const char* parser, parsec_idx length);
int32_t parsec_str_int(const char* parser, parsec_idx length);

#endif /* _PARSEC_H_ */
