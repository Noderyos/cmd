#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define CMD_IMPLEMENTATION
#include "cmd.h"

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

char *output;
size_t output_cursor = 0;

void add_to_output(const char* s, int len) {
    output = realloc(output, output_cursor + len+1);
    strncpy(output+output_cursor, s, len);
    output_cursor += len;
}

void gen_other(const MarkdownData *data) {
    add_to_output(data->content->text, data->content->text_len);
}

void gen_text_start(const MarkdownData *data){UNUSED(data);add_to_output("<p>", 3);}
void gen_text_end(const MarkdownData *data){UNUSED(data);add_to_output("</p>", 4);}

void gen_list_start(const MarkdownData *data){UNUSED(data);add_to_output("<li>", 4);}
void gen_list_end(const MarkdownData *data){UNUSED(data);add_to_output("</li>", 5);}

void gen_code_start(const MarkdownData *data){UNUSED(data);add_to_output("<pre><code>", 11);}
void gen_code_end(const MarkdownData *data){UNUSED(data);add_to_output("</code></pre>", 13);}

void gen_span_start(const MarkdownData *data){UNUSED(data);add_to_output("<span style='color:red;'>", 25);}
void gen_span_end(const MarkdownData *data){UNUSED(data);add_to_output("</span>", 7);}

void gen_image(const MarkdownData *data) {
    char* to_add = malloc(19 + data->alt->text_len + data->content->text_len + 1);
    sprintf(to_add, "<img alt='%.*s' src='%.*s'>", (int)data->alt->text_len, data->alt->text, (int)data->content->text_len, data->content->text);
    add_to_output(to_add, 19 + data->alt->text_len + data->content->text_len);
    free(to_add);
}

void gen_link(const MarkdownData *data) {
    char* to_add = malloc(15 + data->alt->text_len + data->content->text_len + 1);
    sprintf(to_add, "<a href='%.*s'>%.*s</a>", (int)data->content->text_len, data->content->text, (int)data->alt->text_len, data->alt->text);
    add_to_output(to_add, 15 + data->alt->text_len + data->content->text_len);
    free(to_add);
}

void gen_title_start(const MarkdownData *data) {
    char buf[5];
    sprintf(buf, "<h%d>", data->content->type - TOKEN_TITLE1 + 1);
    add_to_output(buf, 4);
}

void gen_title_end(const MarkdownData *data) {
    char buf[6];
    sprintf(buf, "</h%d>", data->content->type - TOKEN_TITLE1 + 1);
    add_to_output(buf, 5);
}

MarkdownFunctionMap fm[] = {
    {TYPE_TEXT, gen_text_start, gen_text_end},
    {TYPE_LIST, gen_list_start, gen_list_end},
    {TYPE_INLINE, gen_span_start, gen_span_end},
    {TYPE_BLOCK, gen_code_start, gen_code_end},
    {TYPE_IMAGE, gen_image, NULL},
    {TYPE_LINK, gen_link, NULL},
    {TYPE_TITLE, gen_title_start, gen_title_end},
    {TYPE_RAW, gen_other, gen_other},
    {TYPE_INVALID, NULL, NULL}
};

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "Usage %s [file to compile]", argv[0]);
        exit(1);
    }

    char *text = read_file(argv[1]);
    Lexer lexer = lexer_init(text, strlen(text));
    Token t = {.type = TOKEN_INVALID};
    generate(fm, &lexer, t, t);

    printf("%.*s", (int)output_cursor, output);

    free(text);
    free(output);
}
