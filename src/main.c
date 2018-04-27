#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <parsec/parsec.h>

#define TOK_NUM 20000
static const char* test = "0.0000D+00    871.5810  1.0000D-06  1.0000D-90  3.2163D-17   2.331  1.1633D+11  0.0000D+00  5.1826D-04";

int main(int argc, const char** args) {
    
    parsec_token tokens[TOK_NUM];
    
    const char* source = test;
    uint64_t length = strlen(test);
    
    
    if(argc == 2) {
        const char* path = args[1];
        FILE* f = fopen(path, "r");
        if(!f) {
            printf("ERROR OPENING FILE\n");
            return -1;
        }
        fseek(f, 0, SEEK_END);
        uint64_t len = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        char* data = malloc(len);
        fread(data, 1, len, f);
        fclose(f);
        
        source = data;
        length = len;
    }
    
    parsec parser;
    parsec_init(&parser, source, length, '%');
    
    printf("-123456 = %d\n", parsec_str_int("-123456", 7));
    printf("-.23D4= %f\n", parsec_str_double("-.23D4", 6));
    
    int parsed = parsec_lex(&parser, tokens, TOK_NUM);
    
    printf("parsed %d tokens\n", parsed);
    for(int i = 0; i < parsed; ++i) {
        if(tokens[i].kind != PARSEC_TOKEN_COMMENT) continue;
        const char* str = source + tokens[i].start+1;
        printf("%.*s\n", tokens[i].length, str);
    }
    
    if(argc == 2) {
        free((void*)source);
    }
}
