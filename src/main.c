#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"

char *read_file(const char *filename){
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;

    return string;
}

char *token_pretty(Token *t){
    switch(t->type){
        case TOKEN_NEWLINE: return "TOKEN_NEWLINE"; 
        case TOKEN_TEXT: return "TOKEN_TEXT";
        case TOKEN_TITLE1: return "TOKEN_TITLE1";
        case TOKEN_TITLE2: return "TOKEN_TITLE2";
        case TOKEN_TITLE3: return "TOKEN_TITLE3";
        case TOKEN_TITLE4: return "TOKEN_TITLE4";
        case TOKEN_INLINE_CODE: return "TOKEN_INLINE_CODE";
        case TOKEN_BLOCK_CODE: return "TOKEN_BLOCK_CODE";
        case TOKEN_LIST: return "TOKEN_LIST";
        case TOKEN_IMAGE: return "TOKEN_IMAGE";
        case TOKEN_LINK: return "TOKEN_LINK";
    }
    return "INVALID";
}

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "Usage %s [file to compile]", argv[0]);
        exit(1);
    }

    char *text = read_file(argv[1]);
    Lexer lexer = lexer_init(text, strlen(text));
    while(lexer.cursor < lexer.content_len){
        Token t = lexer_next(&lexer);
        printf("Type %s str = '%.*s'\n", token_pretty(&t), TOKEN_PRINT(t));
        UNUSED(t);
    }

    free(text);
}