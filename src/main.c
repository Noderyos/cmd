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
        case TOKEN_INVALID: return "TOKEN_INVALID";
        case TOKEN_NEWLINE: return "TOKEN_NEWLINE";
        case TOKEN_TEXT: return "TOKEN_TEXT";
        case TOKEN_TITLE1: return "TOKEN_TITLE1";
        case TOKEN_TITLE2: return "TOKEN_TITLE2";
        case TOKEN_TITLE3: return "TOKEN_TITLE3";
        case TOKEN_TITLE4: return "TOKEN_TITLE4";
        case TOKEN_INLINE_CODE: return "TOKEN_INLINE_CODE";
        case TOKEN_BLOCK_CODE: return "TOKEN_BLOCK_CODE";
        case TOKEN_LIST: return "TOKEN_LIST";
        case TOKEN_URL: return "TOKEN_URL";
        case TOKEN_ALTTEXT: return "TOKEN_ALTTEXT";
        case TOKEN_LINKTEXT: return "TOKEN_LINKTEXT";
    }
    return "INVALID";
}

void generate(Lexer *l, Token t, Token prev);

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "Usage %s [file to compile]", argv[0]);
        exit(1);
    }

    char *text = read_file(argv[1]);
    Lexer lexer = lexer_init(text, strlen(text));
    Token t = {.type = TOKEN_INVALID};
    generate(&lexer, t, t);

    free(text);
}

int was_in_text = 0;

void generate(Lexer *l, Token t, Token prev) {
    if(t.type == TOKEN_INVALID) {
        while(l->cursor < l->content_len) {
            Token next = lexer_next(l);
            generate(l, next, t);
            t = next;
        }
    }

    else if(t.type == TOKEN_TEXT) {
        if(!was_in_text) {
            printf("<p>");
            was_in_text = 1;
        }
        printf("%.*s", (int)t.text_len, t.text);
    }

    else if(t.type == TOKEN_NEWLINE) {
        if(was_in_text)printf("<br>");
        else printf("\n");
    }

    else if(t.type >= TOKEN_TITLE1 && t.type <= TOKEN_TITLE4) {
        if(was_in_text)printf("</p>");
        was_in_text = 1;
        int level = t.type - TOKEN_TITLE1 + 1;
        printf("<h%d>", level);
        prev = t;
        while((t = lexer_next(l)).type != TOKEN_NEWLINE) {
            generate(l, t, prev);
            prev = t;
        }
        printf("</h%d>", level);
        was_in_text = 0;
    }

    else if(t.type == TOKEN_LIST) {
        if(was_in_text) printf("</p>");
        was_in_text = 1;
        printf("<li>");
        prev = t;
        while((t = lexer_next(l)).type != TOKEN_NEWLINE) {
            generate(l, t, prev);
            prev = t;
        }
        was_in_text = 0;
        printf("</li>");
    }

    else if(t.type == TOKEN_BLOCK_CODE) {
        if(was_in_text)printf("</p>");
        was_in_text = 0;
        printf("<code>");
        printf("%.*s", (int)t.text_len, t.text);
        printf("</code>");
    }

    else if(t.type == TOKEN_INLINE_CODE) {
        printf("<span style='color:red'>");
        printf("%.*s", (int)t.text_len, t.text);
        printf("</span>");
    }

    else if(t.type == TOKEN_ALTTEXT) {
        Token next = lexer_next(l);
        ASSERT(next.type == TOKEN_URL, "Alt text without URL")
        printf("<img alt='%.*s' src='%.*s'>", (int)t.text_len, t.text, (int)next.text_len, next.text);
    }

    else if(t.type == TOKEN_LINKTEXT) {
        Token next = lexer_next(l);
        ASSERT(next.type == TOKEN_URL, "Alt text without URL")
        printf("<a href='%.*s'>%.*s</a>", (int)next.text_len, next.text, (int)t.text_len, t.text);
    }
}