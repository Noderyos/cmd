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
char *output;
size_t output_cursor = 0;

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "Usage %s [file to compile]", argv[0]);
        exit(1);
    }

    char *text = read_file(argv[1]);
    Lexer lexer = lexer_init(text, strlen(text));
    Token t = {.type = TOKEN_INVALID};
    generate(&lexer, t, t);
    printf("%s", output);
    free(text);
    free(output);
}

int was_in_text = 0;

void add_to_output(const char* s, int len) {
    output = realloc(output, output_cursor + len);
    strncpy(output+output_cursor, s, len);
    output_cursor += len;
}

void gen_text_start(){add_to_output("<p>", 3);}
void gen_text_end(){add_to_output("</p>", 4);}

void gen_list_start(){add_to_output("<li>", 4);}
void gen_list_end(){add_to_output("</li>", 5);}

void gen_code_start(){add_to_output("<code>", 6);}
void gen_code_end(){add_to_output("</code>", 7);}

void gen_span_start(){add_to_output("<span style='color:red'>", 24);}
void gen_span_end(){add_to_output("</span>", 7);}

void gen_image(Token alt, Token url) {
    char* data = malloc(19 + alt.text_len + url.text_len + 1);
    sprintf(data, "<img alt='%.*s' src='%.*s'>", (int)alt.text_len, alt.text, (int)url.text_len, url.text);
    add_to_output(data, 19 + alt.text_len + url.text_len);
    free(data);
}

void gen_link(Token alt, Token url) {
    char* data = malloc(15 + alt.text_len + url.text_len + 1);
    sprintf(data, "<a href='%.*s'>%.*s</a>", (int)url.text_len, url.text, (int)alt.text_len, alt.text);
    add_to_output(data, 15 + alt.text_len + url.text_len);
    free(data);
}

void gen_title_start(int level) {
    char buf[5];
    sprintf(buf, "<h%d>", level);
    add_to_output(buf, 4);
}

void gen_title_end(int level) {
    char buf[6];
    sprintf(buf, "</h%d>", level);
    add_to_output(buf, 5);
}

void generate(Lexer *l, Token t, Token prev) {
    if(t.type == TOKEN_INVALID) {
        while(l->cursor < l->content_len) {
            Token next = lexer_next(l);
            generate(l, next, t);
            t = next;
        }
        output[output_cursor] = 0;
    }

    else if(t.type == TOKEN_TEXT) {
        if(!was_in_text) {
            gen_text_start();
            was_in_text = 1;
        }
        add_to_output(t.text, t.text_len);
    }

    else if(t.type == TOKEN_NEWLINE) {
        if(was_in_text) add_to_output("<br>", 4);
        else add_to_output("\n", 1);
    }

    else if(t.type >= TOKEN_TITLE1 && t.type <= TOKEN_TITLE4) {
        if(was_in_text) gen_text_end();
        was_in_text = 1;
        int level = t.type - TOKEN_TITLE1 + 1;
        gen_title_start(level);
        prev = t;
        while((t = lexer_next(l)).type != TOKEN_NEWLINE) {
            generate(l, t, prev);
            prev = t;
        }
        gen_title_end(level);
        was_in_text = 0;
    }

    else if(t.type == TOKEN_LIST) {
        if(was_in_text) gen_text_end();
        was_in_text = 1;
        gen_list_start();
        prev = t;
        while((t = lexer_next(l)).type != TOKEN_NEWLINE) {
            generate(l, t, prev);
            prev = t;
        }
        was_in_text = 0;
        gen_list_end();
    }

    else if(t.type == TOKEN_BLOCK_CODE) {
        if(was_in_text) gen_text_end();
        was_in_text = 0;
        gen_code_start();
        add_to_output(t.text, t.text_len);
        gen_code_end();
    }

    else if(t.type == TOKEN_INLINE_CODE) {
        gen_span_start();
        add_to_output(t.text, t.text_len);
        gen_span_end();
    }

    else if(t.type == TOKEN_ALTTEXT) {
        Token next = lexer_next(l);
        ASSERT(next.type == TOKEN_URL, "Alt text without URL")
        gen_image(t, next);
    }

    else if(t.type == TOKEN_LINKTEXT) {
        Token next = lexer_next(l);
        ASSERT(next.type == TOKEN_URL, "Alt text without URL")
        gen_link(t, next);
    }
}